/* vim:ts=4
 *
 * Copyleft 2021  Michał Gawron
 * Marduk Unix Labs, http://mulabs.org/
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Visit http://www.gnu.org/licenses/gpl-3.0.html for more information on licensing.
 */

// Local:
#include "diffie_hellman_exchange.h"

// Neutrino:
#include <neutrino/crypto/utility.h>
#include <neutrino/math/utility.h>
#include <neutrino/stdexcept.h>

// Boost:
#include <boost/random/uniform_int_distribution.hpp>

// Crypto++:
#include <cryptopp/eccrypto.h>
#include <cryptopp/oids.h>
#include <cryptopp/randpool.h>
#include <cryptopp/secblock.h>

// Standard:
#include <cstddef>
#include <random>


namespace neutrino {

using EllipticCurveDomain = CryptoPP::ECDH<CryptoPP::ECP>::Domain;


static void
validate_modp_public_share (ModpScheme::Integer const& public_share,
							ModpScheme::Group const& group)
{
	// RFC3526 MODP groups are safe-prime groups, so q = (p - 1) / 2 is the order
	// of the large prime-order subgroup we want all public shares to belong to.
	auto const subgroup_order = (group.prime - 1) / 2;

	if (public_share <= 1 || public_share >= group.prime - 1)
		throw InvalidArgument ("invalid Diffie-Hellman public share");

	if (powm (public_share, subgroup_order, group.prime) != 1)
		throw InvalidArgument ("invalid Diffie-Hellman public share");
}


static EllipticCurveDomain
make_elliptic_curve_domain (EllipticCurveScheme::EllipticCurve const elliptic_curve)
{
	switch (elliptic_curve)
	{
		case EllipticCurveScheme::EllipticCurve::Secp256r1:
			return EllipticCurveDomain (CryptoPP::ASN1::secp256r1());
	}

	throw InvalidArgument ("unsupported elliptic curve");
}


ModpScheme::ModpScheme (Group const& group):
	_group (group)
{ }


DiffieHellmanValues
ModpScheme::generate_exchange (std::random_device& rnd) const
{
	// Using boost::random::uniform_int_distribution since it supports big integers unlike std::uniform_int_distribution:
	auto distribution = boost::random::uniform_int_distribution<Integer> (Integer (1), pow (Integer (2), _group.bits) - 1);
	auto const secret_value = distribution (rnd);
	auto const public_share_integer = compute_public_or_shared_value (_group.generator, _group.prime, secret_value);

	return DiffieHellmanValues {
		.private_value = serialize_integer (secret_value, _group),
		.public_share = serialize_integer (public_share_integer, _group),
	};
}


Blob
ModpScheme::mix (BlobView const private_value, BlobView const other_exchange_public_share) const
{
	auto const secret_value = deserialize_integer (private_value);
	auto const other_exchange_integer = deserialize_integer (other_exchange_public_share);
	validate_modp_public_share (other_exchange_integer, _group);
	auto const key = compute_public_or_shared_value (other_exchange_integer, _group.prime, secret_value);
	return serialize_integer (key, _group);
}


std::size_t
ModpScheme::max_blob_size() const
{
	return blob_size (_group);
}


Blob
ModpScheme::to_blob (Integer const& integer, size_t const reserve_bytes)
{
	Blob blob;
	blob.reserve (reserve_bytes);
	boost::multiprecision::export_bits (integer.convert_to<boost::multiprecision::cpp_int>(), std::back_inserter (blob), 8);
	return blob;
}


ModpScheme::Integer
ModpScheme::to_integer (BlobView const blob)
{
	boost::multiprecision::cpp_int result;
	boost::multiprecision::import_bits (result, blob.begin(), blob.end());
	return result.convert_to<Integer>();
}


inline ModpScheme::Integer
ModpScheme::compute_public_or_shared_value (Integer const& shared_base,
											Integer const& shared_modulo,
											Integer const& secret)
{
	return powm (shared_base, secret, shared_modulo);
}


std::size_t
ModpScheme::blob_size (Group const& group)
{
	auto const a = msb (group.prime);
	auto const b = a / 8;
	auto const c = a % 8;
	return b + (c > 0 ? 1 : 0);
}


Blob
ModpScheme::serialize_integer (Integer const& integer, Group const& group)
{
	return to_blob (integer, blob_size (group));
}


ModpScheme::Integer
ModpScheme::deserialize_integer (BlobView const blob)
{
	return to_integer (blob);
}


EllipticCurveScheme::EllipticCurveScheme (EllipticCurve const elliptic_curve):
	_elliptic_curve (elliptic_curve)
{ }


DiffieHellmanValues
EllipticCurveScheme::generate_exchange (std::random_device& rnd) const
{
	auto domain = make_elliptic_curve_domain (_elliptic_curve);
	auto entropy = random_blob (64, rnd);
	auto rng = CryptoPP::RandomPool();
	rng.IncorporateEntropy (entropy.data(), entropy.size());
	wipe (entropy);

	auto private_key = CryptoPP::SecByteBlock (domain.PrivateKeyLength());
	auto public_key = CryptoPP::SecByteBlock (domain.PublicKeyLength());
	domain.GenerateKeyPair (rng, private_key, public_key);

	return DiffieHellmanValues {
		.private_value = Blob (private_key.begin(), private_key.end()),
		.public_share = Blob (public_key.begin(), public_key.end()),
	};
}


Blob
EllipticCurveScheme::mix (BlobView const private_value, BlobView const other_exchange_public_share) const
{
	auto domain = make_elliptic_curve_domain (_elliptic_curve);

	if (private_value.size() != domain.PrivateKeyLength())
		throw InvalidArgument ("invalid elliptic-curve Diffie-Hellman private value");

	if (other_exchange_public_share.size() != domain.PublicKeyLength())
		throw InvalidArgument ("invalid elliptic-curve Diffie-Hellman public share");

	auto private_key = CryptoPP::SecByteBlock (private_value.data(), private_value.size());
	auto public_key = CryptoPP::SecByteBlock (other_exchange_public_share.data(), other_exchange_public_share.size());
	auto shared_key = CryptoPP::SecByteBlock (domain.AgreedValueLength());

	if (!domain.Agree (shared_key, private_key, public_key))
		throw InvalidArgument ("invalid elliptic-curve Diffie-Hellman public share");

	return Blob (shared_key.begin(), shared_key.end());
}


std::size_t
EllipticCurveScheme::max_blob_size() const
{
	return make_elliptic_curve_domain (_elliptic_curve).PublicKeyLength();
}

} // namespace neutrino

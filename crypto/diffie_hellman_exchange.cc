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
#include <neutrino/scope_exit.h>
#include <neutrino/stdexcept.h>

// Crypto++:
#include <cryptopp/eccrypto.h>
#include <cryptopp/integer.h>
#include <cryptopp/oids.h>
#include <cryptopp/randpool.h>
#include <cryptopp/secblock.h>

// OpenSSL:
#include <openssl/bn.h>

// Standard:
#include <algorithm>
#include <cstddef>
#include <limits>
#include <memory>
#include <new>
#include <random>


namespace neutrino {

using EllipticCurveDomain = CryptoPP::ECDH<CryptoPP::ECP>::Domain;
using OpenSslInteger = std::unique_ptr<BIGNUM, decltype (&BN_clear_free)>;
using OpenSslContext = std::unique_ptr<BN_CTX, decltype (&BN_CTX_free)>;


/**
 * Convert a nonnegative integer without leaving its encoding in an ordinary buffer.
 */
static OpenSslInteger
to_openssl_integer (ModpScheme::Integer const& integer)
{
	auto const byte_count = integer == 0 ? 1u : msb (integer) / 8 + 1;

	if (byte_count > static_cast<unsigned int> (std::numeric_limits<int>::max()))
		throw InvalidArgument ("MODP integer is too large");

	auto bytes = CryptoPP::SecByteBlock (byte_count);
	boost::multiprecision::export_bits (integer, bytes.begin(), 8);
	auto result = OpenSslInteger (BN_secure_new(), BN_clear_free);

	if (!result)
		throw std::bad_alloc();

	if (!BN_bin2bn (bytes.data(), static_cast<int> (byte_count), result.get()))
		throw Exception ("OpenSSL MODP integer conversion failed");

	return result;
}


static OpenSslInteger
to_openssl_integer (BlobView const bytes)
{
	if (bytes.size() > static_cast<std::size_t> (std::numeric_limits<int>::max()))
		throw InvalidArgument ("MODP integer is too large");

	auto result = OpenSslInteger (BN_secure_new(), BN_clear_free);

	if (!result)
		throw std::bad_alloc();

	if (!BN_bin2bn (bytes.data(), static_cast<int> (bytes.size()), result.get()))
		throw Exception ("OpenSSL MODP integer conversion failed");

	return result;
}


static void
seed_rng (CryptoPP::RandomPool& rng, std::random_device& device)
{
	// Unlike an ordinary Blob, this also wipes partially filled entropy on exceptions.
	auto entropy = CryptoPP::SecByteBlock (64);
	auto distribution = std::uniform_int_distribution<unsigned int> (0, 255);

	for (auto& byte: entropy)
		byte = static_cast<CryptoPP::byte> (distribution (device));

	rng.IncorporateEntropy (entropy.data(), entropy.size());
}


/**
 * Keep private exponents and results out of ordinary cpp_int storage.
 * All OpenSSL integers and the encoded result are cleared through RAII.
 */
static CryptoPP::SecByteBlock
modp_powm (ModpScheme::Integer const& shared_base, BIGNUM const* exponent,
		   ModpScheme::Integer const& shared_modulo)
{
	if (shared_modulo <= 1 || !bit_test (shared_modulo, 0))
		throw InvalidArgument ("MODP modulus must be odd and greater than one");

	// OpenSSL falls back to variable-time exponentiation above this modulus size.
	constexpr auto max_modulus_bits = (std::numeric_limits<int>::max() / BN_BYTES / 256) * BN_BITS2;

	if (msb (shared_modulo) >= static_cast<unsigned int> (max_modulus_bits))
		throw InvalidArgument ("MODP modulus is too large for constant-time exponentiation");

	// Reduction depends only on public operands and also handles negative bases.
	ModpScheme::Integer reduced_base = shared_base % shared_modulo;

	if (reduced_base < 0)
		reduced_base += shared_modulo;

	auto base = to_openssl_integer (reduced_base);
	auto modulo = to_openssl_integer (shared_modulo);
	auto result = OpenSslInteger (BN_secure_new(), BN_clear_free);
	auto context = OpenSslContext (BN_CTX_secure_new(), BN_CTX_free);

	if (!result || !context)
		throw std::bad_alloc();

	// Do not replace this with Boost powm(): its branches depend on secret bits.
	if (BN_mod_exp_mont_consttime (result.get(), base.get(), exponent, modulo.get(), context.get(), nullptr) != 1)
		throw Exception ("OpenSSL MODP exponentiation failed");

	// Preserve the minimal big-endian encoding, including one zero byte for zero.
	auto const byte_count = std::max (1, BN_num_bytes (result.get()));
	auto bytes = CryptoPP::SecByteBlock (static_cast<std::size_t> (byte_count));

	if (BN_bn2binpad (result.get(), bytes.data(), byte_count) != byte_count)
		throw Exception ("OpenSSL MODP result conversion failed");

	return bytes;
}


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
	// Positive exponents below the subgroup order q cannot produce 1 for a generator of order q.
	Integer const subgroup_order = (_group.prime - 1) / 2;
	Integer const max_by_bits = pow (Integer (2), _group.bits) - 1;
	Integer const max_secret = std::min (max_by_bits, Integer (subgroup_order - 1));

	if (max_secret < 1)
		throw InvalidArgument ("MODP group has no valid private exponents");

	auto rng = CryptoPP::RandomPool();
	seed_rng (rng, rnd);

	auto const upper_bound = to_openssl_integer (max_secret);
	auto exponent = to_openssl_integer (BlobView {});
	auto const byte_count = BN_num_bytes (upper_bound.get());
	auto secret_bytes = CryptoPP::SecByteBlock (static_cast<std::size_t> (byte_count));
	auto const high_byte_mask = static_cast<CryptoPP::byte> (0xffu >> (7 - msb (max_secret) % 8));

	// Mask unused high bits and reject zero or out-of-range candidates, avoiding modulo bias.
	// Both the candidate bytes and the BIGNUM are wiped, including on exceptions.
	do {
		rng.GenerateBlock (secret_bytes.data(), secret_bytes.size());
		secret_bytes[0] &= high_byte_mask;

		if (!BN_bin2bn (secret_bytes.data(), byte_count, exponent.get()))
			throw Exception ("OpenSSL MODP integer conversion failed");
	} while (BN_is_zero (exponent.get()) || BN_cmp (exponent.get(), upper_bound.get()) > 0);

	BN_set_flags (exponent.get(), BN_FLG_CONSTTIME);
	auto const public_share = modp_powm (_group.generator, exponent.get(), _group.prime);
	// Keep the existing minimal encoding of the private value.
	auto const first_byte = std::find_if (secret_bytes.begin(), secret_bytes.end(), [](auto byte) { return byte != 0; });

	return DiffieHellmanValues {
		.private_value = Secure<Blob> (first_byte, secret_bytes.end()),
		.public_share = Blob (public_share.data(), public_share.size()),
	};
}


Blob
ModpScheme::mix (BlobView const private_value, BlobView const other_exchange_public_share) const
{
	auto secret = to_openssl_integer (private_value);
	Integer const subgroup_order = (_group.prime - 1) / 2;

	if (subgroup_order <= 1)
		throw InvalidArgument ("invalid MODP Diffie-Hellman private value");

	auto const order = to_openssl_integer (subgroup_order);

	// Match generate_exchange()'s range without copying the secret into a cpp_int.
	if (BN_is_zero (secret.get()) ||
		BN_cmp (secret.get(), order.get()) >= 0 ||
		static_cast<std::size_t> (BN_num_bits (secret.get())) > _group.bits)
	{
		throw InvalidArgument ("invalid MODP Diffie-Hellman private value");
	}

	BN_set_flags (secret.get(), BN_FLG_CONSTTIME);
	auto const other_exchange_integer = to_integer (other_exchange_public_share);
	validate_modp_public_share (other_exchange_integer, _group);
	auto const key = modp_powm (other_exchange_integer, secret.get(), _group.prime);
	return Blob (key.data(), key.size());
}


std::size_t
ModpScheme::max_blob_size() const
{
	return blob_size (_group);
}


Blob
ModpScheme::to_blob (Integer const& integer, size_t const reserve_bytes)
{
	// export_bits() exports the magnitude. Reserve enough space before copying any secret.
	auto const capacity = std::max ({ std::size_t (1), reserve_bytes, integer.backend().size() * sizeof (*integer.backend().limbs()) });
	auto bytes = CryptoPP::SecByteBlock (capacity);
	auto const end = boost::multiprecision::export_bits (integer, bytes.begin(), 8);
	return Blob (bytes.data(), static_cast<std::size_t> (end - bytes.begin()));
}


ModpScheme::Integer
ModpScheme::to_integer (BlobView const blob)
{
	// Some Boost versions access invalid memory when importing an empty range.
	if (blob.empty())
		return Integer (0);

	// Fill the final backend directly: import_bits() creates another ordinary cpp_int.
	Integer result;
	ScopeExit cleanup ([&] { wipe (result); });
	auto& backend = result.backend();
	using Limb = boost::multiprecision::limb_type;
	constexpr auto limb_bytes = sizeof (Limb);
	auto const limb_count = blob.size() / limb_bytes + (blob.size() % limb_bytes != 0);

	if (limb_count > std::numeric_limits<unsigned int>::max())
		throw InvalidArgument ("MODP integer is too large");

	backend.resize (static_cast<unsigned int> (limb_count), static_cast<unsigned int> (limb_count));
	std::fill_n (backend.limbs(), limb_count, Limb (0));

	for (std::size_t i = 0; i < blob.size(); ++i)
		backend.limbs()[i / limb_bytes] |= Limb (blob[blob.size() - i - 1]) << (8 * (i % limb_bytes));

	backend.normalize();
	// An explicit move keeps cleanup bound to the moved-from storage, even with NRVO enabled.
	return Integer (std::move (result));
}


ModpScheme::Integer
ModpScheme::powm (Integer const& shared_base, Integer const& secret, Integer const& shared_modulo)
{
	if (secret < 0)
		throw InvalidArgument ("MODP exponent must be nonnegative");

	auto exponent = to_openssl_integer (secret);
	BN_set_flags (exponent.get(), BN_FLG_CONSTTIME);
	auto const bytes = modp_powm (shared_base, exponent.get(), shared_modulo);
	// The raw Integer returned by this public API belongs to the caller.
	return to_integer (BlobView (bytes.data(), bytes.size()));
}


std::size_t
ModpScheme::blob_size (Group const& group)
{
	// msb() returns the zero-based index of the highest set bit.
	return msb (group.prime) / 8 + 1;
}


EllipticCurveScheme::EllipticCurveScheme (EllipticCurve const elliptic_curve):
	_elliptic_curve (elliptic_curve)
{ }


DiffieHellmanValues
EllipticCurveScheme::generate_exchange (std::random_device& rnd) const
{
	auto domain = make_elliptic_curve_domain (_elliptic_curve);
	auto rng = CryptoPP::RandomPool();
	seed_rng (rng, rnd);

	auto private_key = CryptoPP::SecByteBlock (domain.PrivateKeyLength());
	auto public_key = CryptoPP::SecByteBlock (domain.PublicKeyLength());
	domain.GenerateKeyPair (rng, private_key, public_key);

	return DiffieHellmanValues {
		.private_value = Secure<Blob> (private_key.data(), private_key.size()),
		.public_share = Blob (public_key.begin(), public_key.end()),
	};
}


Blob
EllipticCurveScheme::mix (BlobView const private_value, BlobView const other_exchange_public_share) const
{
	auto domain = make_elliptic_curve_domain (_elliptic_curve);

	if (private_value.size() != domain.PrivateKeyLength())
		throw InvalidArgument ("invalid elliptic-curve Diffie-Hellman private value");

	// A correctly sized scalar can still be zero or outside the subgroup order.
	auto const private_scalar = CryptoPP::Integer (private_value.data(), private_value.size());

	if (!private_scalar.IsPositive() || private_scalar >= domain.GetGroupParameters().GetSubgroupOrder())
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

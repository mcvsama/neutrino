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

// Botan:
#include <botan/bigint.h>
#include <botan/dl_group.h>
#include <botan/ec_apoint.h>
#include <botan/ec_group.h>
#include <botan/ec_scalar.h>
#include <botan/ecdh.h>
#include <botan/hmac_drbg.h>
#include <botan/system_rng.h>
#include <botan/version.h>

// Standard:
#include <algorithm>
#include <cstddef>
#include <limits>
#include <random>


namespace neutrino {

static_assert (BOTAN_VERSION_CODE >= BOTAN_VERSION_CODE_FOR (3, 13, 0), "ECDH requires Botan 3.13 or newer");

static constexpr std::size_t max_modp_bits = 16384;


/**
 * Convert without leaving an intermediate encoding in an ordinary Blob.
 * Botan's BigInt stores its limbs in secure memory.
 */
static Botan::BigInt
to_botan_integer (ModpScheme::Integer const& integer)
{
	auto bytes = Botan::secure_vector<uint8_t> (std::max (std::size_t (1), integer.backend().size() * sizeof (*integer.backend().limbs())));
	auto const end = boost::multiprecision::export_bits (integer, bytes.begin(), 8);
	auto result = Botan::BigInt::from_bytes ({ bytes.data(), static_cast<std::size_t> (end - bytes.begin()) });

	if (integer < 0)
		result.set_sign (Botan::BigInt::Negative);

	return result;
}


static void
validate_modp_modulus (ModpScheme::Integer const& modulus)
{
	// Botan's DL_Group supports odd moduli of 3 through 16384 bits.
	if (modulus <= 3 || !bit_test (modulus, 0) || msb (modulus) >= max_modp_bits)
		throw InvalidArgument ("unsupported MODP modulus");
}


static Botan::DL_Group
make_modp_group (ModpScheme::Group const& group)
{
	validate_modp_modulus (group.prime);

	if (group.generator < 2 || group.generator >= group.prime)
		throw InvalidArgument ("invalid MODP generator");

	return Botan::DL_Group (to_botan_integer (group.prime), to_botan_integer (group.generator));
}


static void
validate_modp_public_share (Botan::BigInt const& public_share, Botan::DL_Group const& group,
							Botan::BigInt const& subgroup_order)
{
	if (public_share <= 1 || public_share >= group.get_p() - 1)
		throw InvalidArgument ("invalid Diffie-Hellman public share");

	// Safe-prime groups have a subgroup of order q = (p - 1) / 2.
	if (group.power_b_p (public_share, subgroup_order, group.p_bits()) != 1)
		throw InvalidArgument ("invalid Diffie-Hellman public share");
}


static Botan::EC_Group
make_elliptic_curve_group (EllipticCurveScheme::EllipticCurve const curve)
{
	if (curve != EllipticCurveScheme::EllipticCurve::Secp256r1)
		throw InvalidArgument ("unsupported elliptic curve");

	auto const group = Botan::EC_Group::from_name ("secp256r1");

	// Legacy ECC has known side channels; do not silently fall back to it.
	if (group.engine() == Botan::EC_Group_Engine::Legacy)
		throw Exception ("Botan P-256 requires the constant-time pcurves backend");

	return group;
}


static void
seed_rng (Botan::RandomNumberGenerator& rng, std::random_device& device)
{
	// Seed only from the caller's device; secure storage also wipes on exceptions.
	auto entropy = Botan::secure_vector<uint8_t> (64);
	auto distribution = std::uniform_int_distribution<unsigned int> (0, 255);

	for (auto& byte: entropy)
		byte = static_cast<uint8_t> (distribution (device));

	rng.add_entropy (entropy);
}


ModpScheme::ModpScheme (Group const& group):
	_group (group)
{ }


DiffieHellmanValues
ModpScheme::generate_exchange (std::random_device& rnd) const
{
	// Positive exponents below the subgroup order q cannot produce 1 for a generator of order q.
	Integer const subgroup_order = (_group.prime - 1) / 2;
	Integer max_secret = subgroup_order - 1;

	if (_group.bits == 0 || max_secret < 1)
		throw InvalidArgument ("MODP group has no valid private exponents");

	// Only construct 2^bits when it tightens the subgroup bound. This also bounds
	// the shift by the existing group's size, even if bits is as large as SIZE_MAX.
	if (_group.bits <= msb (max_secret))
		max_secret = (Integer (1) << _group.bits) - 1;

	auto const group = make_modp_group (_group);
	auto rng = Botan::HMAC_DRBG ("SHA-256");
	seed_rng (rng, rnd);

	auto const exponent = Botan::BigInt::random_integer (rng, 1, to_botan_integer (max_secret) + 1);

	// Use a public upper bound so exponentiation does not reveal the secret's bit length.
	auto const exponent_bits = std::min (_group.bits, group.p_bits());
	auto const public_share = group.power_g_p (exponent, exponent_bits).serialize();
	auto const private_value = exponent.serialize<Botan::secure_vector<uint8_t>>();

	return DiffieHellmanValues {
		.private_value = Secure<Blob> (private_value.data(), private_value.size()),
		.public_share = Blob (public_share.begin(), public_share.end()),
	};
}


Blob
ModpScheme::mix (BlobView const private_value, BlobView const other_exchange_public_share) const
{
	// Bound both encodings before importing either one into a big integer.
	// Leading zeros must not allow oversized inputs to bypass the group limit.
	auto const max_size = max_blob_size();

	if (private_value.empty() || private_value.size() > max_size)
		throw InvalidArgument ("invalid MODP Diffie-Hellman private value length");

	if (other_exchange_public_share.empty() || other_exchange_public_share.size() > max_size)
		throw InvalidArgument ("invalid MODP Diffie-Hellman public share length");

	auto const secret = Botan::BigInt::from_bytes ({ private_value.data(), private_value.size() });
	Integer const subgroup_order = (_group.prime - 1) / 2;

	if (subgroup_order <= 1 || secret.is_zero() || secret >= to_botan_integer (subgroup_order) || secret.bits() > _group.bits)
		throw InvalidArgument ("invalid MODP Diffie-Hellman private value");

	auto const group = make_modp_group (_group);
	auto const other_exchange_integer = Botan::BigInt::from_bytes ({ other_exchange_public_share.data(), other_exchange_public_share.size() });
	validate_modp_public_share (other_exchange_integer, group, to_botan_integer (subgroup_order));

	// Both methods use the same public bound, independent of the accepted private value.
	auto const exponent_bits = std::min (_group.bits, group.p_bits());
	auto const key = group.power_b_p (other_exchange_integer, secret, exponent_bits).serialize<Botan::secure_vector<uint8_t>>();
	return Blob (key.begin(), key.end());
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
	auto bytes = Botan::secure_vector<uint8_t> (capacity);
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
ModpScheme::powm (Integer const& shared_base, Integer const& secret, Integer const& shared_modulo, std::size_t const max_secret_bits)
{
	if (secret < 0)
		throw InvalidArgument ("MODP exponent must be nonnegative");

	validate_modp_modulus (shared_modulo);

	if (max_secret_bits > max_modp_bits)
		throw InvalidArgument ("unsupported MODP exponent bit limit");

	if (secret != 0 && msb (secret) >= max_secret_bits)
		throw InvalidArgument ("MODP exponent exceeds its public bit limit");

	// power_b_p() uses the explicit base; the group's generator is unused here.
	auto const group = Botan::DL_Group (to_botan_integer (shared_modulo), 2);
	// Only public operands are reduced, including a possibly negative base.
	Integer reduced_base = shared_base % shared_modulo;

	if (reduced_base < 0)
		reduced_base += shared_modulo;

	// Never infer this loop bound from the secret or fall back to Botan::power_mod().
	auto const result = group.power_b_p (to_botan_integer (reduced_base), to_botan_integer (secret), max_secret_bits);
	auto const bytes = result.serialize<Botan::secure_vector<uint8_t>>();
	// The ordinary Integer returned by this public API belongs to the caller.
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
	auto const group = make_elliptic_curve_group (_elliptic_curve);
	auto rng = Botan::HMAC_DRBG ("SHA-256");
	seed_rng (rng, rnd);
	auto const key = Botan::ECDH_PrivateKey (rng, group);
	auto const private_key = key.raw_private_key_bits();
	auto const public_key = key.public_value();

	return DiffieHellmanValues {
		.private_value = Secure<Blob> (private_key.data(), private_key.size()),
		.public_share = Blob (public_key.begin(), public_key.end()),
	};
}


Blob
EllipticCurveScheme::mix (BlobView const private_value, BlobView const other_exchange_public_share) const
{
	auto const group = make_elliptic_curve_group (_elliptic_curve);
	auto const scalar = Botan::EC_Scalar::deserialize (group, { private_value.data(), private_value.size() });

	if (!scalar || scalar->is_zero())
		throw InvalidArgument ("invalid elliptic-curve Diffie-Hellman private value");

	// Strict decoding excludes infinity and off-curve points.
	auto const point = Botan::EC_AffinePoint::deserialize_uncompressed (
		group, { other_exchange_public_share.data(), other_exchange_public_share.size() }
	);

	if (!point)
		throw InvalidArgument ("invalid elliptic-curve Diffie-Hellman public share");

	// P-256 has cofactor 1, so this is the raw ECDH secret. The system RNG
	// enables scalar blinding and point rerandomization during multiplication.
	auto const shared = point->mul_x_only (*scalar, Botan::system_rng());
	return Blob (shared.begin(), shared.end());
}


std::size_t
EllipticCurveScheme::max_blob_size() const
{
	return 1 + 2 * make_elliptic_curve_group (_elliptic_curve).get_p_bytes();
}

} // namespace neutrino

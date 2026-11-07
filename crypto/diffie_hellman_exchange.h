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

#ifndef NEUTRINO__CRYPTO__DIFFIE_HELLMAN_EXCHANGE_H__INCLUDED
#define NEUTRINO__CRYPTO__DIFFIE_HELLMAN_EXCHANGE_H__INCLUDED

// Neutrino:
#include <neutrino/crypto/modp.h>
#include <neutrino/crypto/secure.h>
#include <neutrino/blob.h>

// Boost:
#include <boost/multiprecision/cpp_int.hpp>

// Standard:
#include <cstddef>
#include <cstdint>
#include <random>


namespace neutrino {

struct DiffieHellmanValues
{
	Secure<Blob>	private_value;
	Blob			public_share;
};


class ModpScheme
{
  public:
	using Integer = boost::multiprecision::cpp_int;
	using Group = ModularExponentialGroup<Integer>;

  public:
	explicit
	ModpScheme (Group const& = RFC3526_Group14<Integer>);

	/**
	 * Generate a private exponent in [1, min(2^bits - 1, q - 1)], where q = (p - 1) / 2.
	 * Assumes valid safe-prime group parameters with a generator of order q.
	 * Group moduli must have 3 through 16384 bits, as supported by Botan's DL_Group.
	 * Throw InvalidArgument if the private-exponent interval is empty or group parameters are unsupported.
	 */
	[[nodiscard]]
	DiffieHellmanValues
	generate_exchange (std::random_device&) const;

	/**
	 * Compute a shared secret using a private exponent in the same range as generate_exchange().
	 * Both input encodings must contain 1 through max_blob_size() bytes.
	 * Leading zero padding is allowed within this limit.
	 * Group parameters must satisfy the same requirements as generate_exchange().
	 * Throw InvalidArgument if the private exponent or the peer's public share is invalid.
	 */
	[[nodiscard]]
	Blob
	mix (BlobView private_value, BlobView other_exchange_public_share) const;

	[[nodiscard]]
	std::size_t
	max_blob_size() const;

	[[nodiscard]]
	Group const&
	group() const
		{ return _group; }

  public:
	[[nodiscard]]
	static Blob
	to_blob (Integer const&, size_t reserve_bytes);

	/**
	 * Decode an unsigned big-endian integer; an empty encoding represents zero.
	 * The returned ordinary Integer is caller-owned and must be wiped by the caller if secret.
	 */
	[[nodiscard]]
	static Integer
	to_integer (BlobView blob);

	/**
	 * Compute shared_base^secret modulo an odd shared_modulo greater than one.
	 * secret must be nonnegative. Uses Botan's Montgomery exponentiation;
	 * operand conversion and running time depend on operand bit lengths.
	 * For exchange operations, the exponentiation loop uses a public bit-length bound.
	 * Unsupported parameters are rejected rather than using variable-time Boost powm().
	 * The returned ordinary Integer is caller-owned and must be wiped by the caller if secret.
	 */
	[[nodiscard]]
	static Integer
	compute_public_or_shared_value (Integer const& shared_base,
									Integer const& shared_modulo,
									Integer const& secret)
		{ return powm (shared_base, secret, shared_modulo); }

  private:
	[[nodiscard]]
	static Integer
	powm (Integer const& shared_base, Integer const& secret, Integer const& shared_modulo);

	[[nodiscard]]
	static std::size_t
	blob_size (Group const&);

  private:
	Group _group;
};


class EllipticCurveScheme
{
  public:
	enum class EllipticCurve: uint8_t
	{
		Secp256r1 = 0x01,
	};

  public:
	explicit
	EllipticCurveScheme (EllipticCurve = EllipticCurve::Secp256r1);

	/**
	 * Generate a private scalar and public share using entropy from the given device.
	 * P-256 uses a 32-byte big-endian scalar and a 65-byte uncompressed SEC1 point.
	 */
	[[nodiscard]]
	DiffieHellmanValues
	generate_exchange (std::random_device&) const;

	/**
	 * Compute a shared secret using a fixed-length, unsigned big-endian private scalar.
	 * The peer share must be a non-infinite, on-curve point in uncompressed SEC1 format.
	 * Return the fixed-length big-endian x coordinate (32 bytes for P-256).
	 * Throw InvalidArgument for invalid peer points, key lengths or a private scalar outside [1, n - 1],
	 * where n is the curve's subgroup order.
	 */
	[[nodiscard]]
	Blob
	mix (BlobView private_value, BlobView other_exchange_public_share) const;

	[[nodiscard]]
	std::size_t
	max_blob_size() const;

	[[nodiscard]]
	EllipticCurve
	elliptic_curve() const
		{ return _elliptic_curve; }

  private:
	EllipticCurve _elliptic_curve;
};


template<class Scheme>
	class DiffieHellmanExchange
	{
	  public:
		using SchemeType = Scheme;

	  public:
		// Ctor
		explicit
		DiffieHellmanExchange (std::random_device&);

		// Ctor
		explicit
		DiffieHellmanExchange (std::random_device&, Scheme const&);

		/**
		 * Return public share that needs to be sent to the other party.
		 */
		[[nodiscard]]
		BlobView
		public_share() const
			{ return _public_share; }

		/**
		 * Finalize exchange and return secret key to use.
		 * It's best to hash the result and use the hashed value as a key,
		 * because the direct result is non-uniformly random. Hence it's
		 * called "_with_weak_bits".
		 */
		[[nodiscard]]
		Blob
		compute_key_with_weak_bits (BlobView other_exchange_public_share) const;

		[[nodiscard]]
		std::size_t
		max_blob_size() const
			{ return _scheme.max_blob_size(); }

		[[nodiscard]]
		SchemeType const&
		scheme() const
			{ return _scheme; }

	  private:
		SchemeType		_scheme;
		Secure<Blob>	_private_value;
		Blob			_public_share;
	};


template<class Scheme>
	inline
	DiffieHellmanExchange<Scheme>::DiffieHellmanExchange (std::random_device& rnd):
		DiffieHellmanExchange (rnd, Scheme())
	{ }


template<class Scheme>
	inline
	DiffieHellmanExchange<Scheme>::DiffieHellmanExchange (std::random_device& rnd, Scheme const& scheme):
		_scheme (scheme)
	{
		auto generated = _scheme.generate_exchange (rnd);
		_private_value = std::move (generated.private_value);
		_public_share = std::move (generated.public_share);
	}


template<class Scheme>
	inline Blob
	DiffieHellmanExchange<Scheme>::compute_key_with_weak_bits (BlobView const other_exchange_public_share) const
	{
		return _scheme.mix (*_private_value, other_exchange_public_share);
	}

} // namespace neutrino

#endif

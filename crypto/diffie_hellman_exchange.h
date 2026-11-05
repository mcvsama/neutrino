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

	[[nodiscard]]
	DiffieHellmanValues
	generate_exchange (std::random_device&) const;

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
	 */
	[[nodiscard]]
	static Integer
	to_integer (BlobView blob);

	[[nodiscard]]
	static Integer
	compute_public_or_shared_value (Integer const& shared_base,
									Integer const& shared_modulo,
									Integer const& secret);

  private:
	[[nodiscard]]
	static std::size_t
	blob_size (Group const&);

	[[nodiscard]]
	static Blob
	serialize_integer (Integer const&, Group const&);

	[[nodiscard]]
	static Integer
	deserialize_integer (BlobView);

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

	[[nodiscard]]
	DiffieHellmanValues
	generate_exchange (std::random_device&) const;

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

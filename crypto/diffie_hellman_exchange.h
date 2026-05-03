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
#include <neutrino/blob.h>

// Boost:
#include <boost/random.hpp>
#include <boost/multiprecision/cpp_int.hpp>

// Standard:
#include <cstddef>
#include <random>


namespace neutrino {

class DiffieHellmanExchange
{
  public:
	using Integer = boost::multiprecision::cpp_int;
	using Group = ModularExponentialGroup<Integer>;

	// Ctor
	DiffieHellmanExchange (std::random_device&, Group const& = RFC3526_Group14<Integer>);

	// Dtor
	~DiffieHellmanExchange();

	/**
	 * Return public share that needs to be sent to the other party.
	 */
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

  public:
	/**
	 * Return number of bytes required to store the exchange integer and the computed key.
	 * This depends on the shared modulo value.
	 */
	[[nodiscard]]
	static std::size_t
	max_blob_size (Group const&);

	[[nodiscard]]
	static Blob
	to_blob (Integer const&, size_t reserve_bytes);

	[[nodiscard]]
	static Integer
	to_integer (BlobView const blob);

	[[nodiscard]]
	static Integer
	mix (Integer const& shared_base,
		 Integer const& shared_modulo,
		 Integer const& secret);

  private:
	Group		_group;
	std::size_t	_max_blob_size;
	Blob		_public_share;
	// Must stay secret:
	Integer		_secret_value;
};

} // namespace neutrino

#endif

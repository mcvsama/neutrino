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
#include <boost/random/random_device.hpp>
#include <boost/multiprecision/cpp_int.hpp>

// Standard:
#include <cstddef>


namespace neutrino {

class DiffieHellmanExchange
{
  public:
	using Integer = boost::multiprecision::cpp_int;
	using Group = ModularExponentialGroup<Integer>;

  public:
	// Ctor
	explicit
	DiffieHellmanExchange (boost::random::random_device&);

	// Ctor
	explicit
	DiffieHellmanExchange (boost::random::random_device&, Group const&);

	// Dtor
	~DiffieHellmanExchange();

	/**
	 * Return value for exchange with the other party.
	 * Call this each time for new DHE exchange. It generates new secret value on each call.
	 */
	[[nodiscard]]
	Blob
	generate_exchange_blob();

	/**
	 * Finalize exchange and return secret key to use.
	 * It's best to hash the result and use the hashed value as a key.
	 */
	[[nodiscard]]
	Blob
	calculate_key_with_weak_bits (Blob const& other_exchange_blob) const;

	/**
	 * Return number of bytes required to store the exchange integer and the calculated key.
	 * This depends on the shared modulo value.
	 */
	[[nodiscard]]
	size_t
	max_blob_size() const;

  public:
	[[nodiscard]]
	static Blob
	to_blob (Integer const&, size_t reserve_bytes);

	[[nodiscard]]
	static Integer
	to_integer (BlobView const blob);

  private:
	[[nodiscard]]
	static Integer
	mix (Integer const& shared_base,
		 Integer const& shared_modulo,
		 Integer const& secret);

  private:
	boost::random::random_device&	_random_device;
	Group							_group;
	std::optional<Integer>			_secret_value;
};

} // namespace neutrino

#endif


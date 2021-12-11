/* vim:ts=4
 *
 * Copyleft 2012…2016  Michał Gawron
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

// Standard:
#include <cstddef>

// Boost:
#include <boost/random.hpp>
#include <boost/random/random_device.hpp>
#include <boost/multiprecision/cpp_int.hpp>

// Neutrino:
#include <neutrino/blob.h>


namespace neutrino {

class DiffieHellmanExchange
{
  public:
	using Integer = boost::multiprecision::cpp_int;

	class Parameters
	{
	  public:
		size_t	bits;
		Integer	shared_base;
		Integer	shared_modulo;
	};

	static inline Parameters const kNeutrino2019 {
		.bits			= 2048,
		.shared_base	= 2,
		.shared_modulo	= math::mersenne_prime<Integer> (2203u),
	};

  public:
	// Ctor
	explicit
	DiffieHellmanExchange (boost::random::random_device&);

	// Ctor
	explicit
	DiffieHellmanExchange (boost::random::random_device&, Parameters const&);

	// Dtor
	~DiffieHellmanExchange();

	/**
	 * Return value for exchange with the other party.
	 */
	Integer
	exchange_integer()
		{ return _exchange_integer; }

	/**
	 * Return value for exchange with the other party.
	 */
	Blob
	exchange_blob();

	/**
	 * Finalize exchange and return secret key to use.
	 * It's best to hash the result and use the hashed value as a key.
	 */
	Integer
	calculate_key_with_weak_bits (Integer const& other_exchange_integer) const;

	/**
	 * Finalize exchange and return secret key to use.
	 * It's best to hash the result and use the hashed value as a key.
	 */
	Blob
	calculate_key_with_weak_bits (Blob const& other_exchange_blob) const;

	/**
	 * Return number of bytes required to store the exchange integer and the calculated key.
	 * This depends on the shared modulo value.
	 */
	size_t
	max_blob_size() const
		{ return 8 * msb (_shared_modulo); }

  private:
	void
	generate_exchange_integer (boost::random::random_device&);

	static Integer
	mix (Integer const& shared_base,
		 Integer const& shared_modulo,
		 Integer const& secret);

  private:
	uint32_t	_bits;
	Integer		_shared_base;
	Integer		_shared_modulo;
	Integer		_exchange_integer;
	Integer		_secret_value;
};

} // namespace neutrino

#endif


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

	enum class Standard
	{
		Xefis2019,
	};

  public:
	// Ctor
	explicit
	DiffieHellmanExchange (boost::random::random_device&);

	// Ctor
	explicit
	DiffieHellmanExchange (boost::random::random_device&, Standard standard);

	// Ctor
	explicit
	DiffieHellmanExchange (boost::random::random_device&,
						   uint32_t bits,
						   Integer const& shared_base,
						   Integer const& shared_prime);

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
	 */
	Integer
	calculate_key (Integer const& other_exchange_integer) const;

	/**
	 * Finalize exchange and return secret key to use.
	 */
	Blob
	calculate_key (Blob const& other_exchange_blob) const;

  private:
	void
	generate_exchange_integer (boost::random::random_device&);

	static Integer
	mix (Integer const& shared_prime,
		 Integer const& shared_base,
		 Integer const& secret);

  private:
	uint32_t	_bits;
	Integer		_shared_base;
	Integer		_shared_prime;
	Integer		_exchange_integer;
	Integer		_secure_value;
};

} // namespace neutrino

#endif


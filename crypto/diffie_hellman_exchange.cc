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

// Standard:
#include <cstddef>

// Neutrino:
#include <neutrino/crypto/utility.h>
#include <neutrino/math/utility.h>

// Local:
#include "diffie_hellman_exchange.h"


namespace neutrino {

DiffieHellmanExchange::DiffieHellmanExchange (boost::random::random_device& random_device):
	DiffieHellmanExchange (random_device, Standard::Xefis2019)
{ }


DiffieHellmanExchange::DiffieHellmanExchange (boost::random::random_device& random_device,
											  Standard standard)
{
	switch (standard)
	{
		case Standard::Xefis2019:
			_bits = 2048;
			_shared_base = 2;
			_shared_modulo = math::mersenne_prime<Integer> (2203u);
			break;
	}

	generate_exchange_integer (random_device);
}


DiffieHellmanExchange::DiffieHellmanExchange (boost::random::random_device& random_device,
											  uint32_t bits,
											  Integer const& shared_base,
											  Integer const& shared_modulo):
	_bits (bits),
	_shared_base (shared_base),
	_shared_modulo (shared_modulo)
{
	generate_exchange_integer (random_device);
}


DiffieHellmanExchange::~DiffieHellmanExchange()
{
	// Wipe value so that it doesn't remain in memory.
	// There might be other copies, too, but at least we can wipe this one.
	wipe (_secret_value);
}


Blob
DiffieHellmanExchange::exchange_blob()
{
	Blob blob;
	blob.reserve (max_blob_size());
	boost::multiprecision::export_bits (exchange_integer(), std::back_inserter (blob), 8);
	return blob;
}


DiffieHellmanExchange::Integer
DiffieHellmanExchange::calculate_key (Integer const& other_exchange_integer) const
{
	return mix (other_exchange_integer, _shared_modulo, _secret_value);
}


Blob
DiffieHellmanExchange::calculate_key (Blob const& other_exchange_blob) const
{
	Integer other_exchange_integer;
	boost::multiprecision::import_bits (other_exchange_integer, other_exchange_blob.begin(), other_exchange_blob.end());

	auto key = calculate_key (other_exchange_integer);

	Blob blob;
	blob.reserve (max_blob_size());
	boost::multiprecision::export_bits (key, std::back_inserter (blob), 8);
	return blob;
}


void
DiffieHellmanExchange::generate_exchange_integer (boost::random::random_device& random_device)
{
	auto distribution = boost::random::uniform_int_distribution<Integer> (Integer (1), pow (Integer (2), _bits) - 1);
	_secret_value = distribution (random_device);
	_exchange_integer = mix (_shared_base, _shared_modulo, _secret_value);
}


inline DiffieHellmanExchange::Integer
DiffieHellmanExchange::mix (Integer const& shared_base,
							Integer const& shared_modulo,
							Integer const& secret)
{
	return powm (shared_base, secret, shared_modulo);
}

} // namespace neutrino


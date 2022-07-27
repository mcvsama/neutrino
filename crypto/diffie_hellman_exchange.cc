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

// Standard:
#include <cstddef>


namespace neutrino {

DiffieHellmanExchange::DiffieHellmanExchange (boost::random::random_device& random_device):
	DiffieHellmanExchange (random_device, RFC3526_Group14<Integer>)
{ }


DiffieHellmanExchange::DiffieHellmanExchange (boost::random::random_device& random_device,
											  Group const& group):
	_random_device (random_device),
	_group (group)
{ }


DiffieHellmanExchange::~DiffieHellmanExchange()
{
	// Wipe value so that it doesn't remain in memory.
	// There might be other copies, too, but at least we can wipe this one.
	if (_secret_value)
		wipe (*_secret_value);
}


Blob
DiffieHellmanExchange::generate_exchange_blob()
{
	auto distribution = boost::random::uniform_int_distribution<Integer> (Integer (1), pow (Integer (2), _group.bits) - 1);
	_secret_value = distribution (_random_device);
	auto const exchange_integer = mix (_group.generator, _group.prime, *_secret_value);
	return to_blob (exchange_integer, max_blob_size());
}


Blob
DiffieHellmanExchange::calculate_key_with_weak_bits (Blob const& other_exchange_blob) const
{
	if (!_secret_value)
		throw std::runtime_error ("called DiffieHellmanExchange::calculate_key_with_weak_bits() without first calling generate_exchange_blob()");

	auto const other_exchange_integer = to_integer (other_exchange_blob);
	auto const key = mix (other_exchange_integer, _group.prime, *_secret_value);
	return to_blob (key, max_blob_size());
}


size_t
DiffieHellmanExchange::max_blob_size() const
{
	auto const a = msb (_group.prime);
	auto const b = a / 8;
	auto const c = a % 8;
	return b + (c > 0 ? 1 : 0);
}


Blob
DiffieHellmanExchange::to_blob (Integer const& integer, size_t const reserve_bytes)
{
	Blob blob;
	blob.reserve (reserve_bytes);
	boost::multiprecision::export_bits (integer.convert_to<boost::multiprecision::cpp_int>(), std::back_inserter (blob), 8);
	return blob;
}


DiffieHellmanExchange::Integer
DiffieHellmanExchange::to_integer (BlobView const blob)
{
	boost::multiprecision::cpp_int result;
	boost::multiprecision::import_bits (result, blob.begin(), blob.end());
	return result.convert_to<Integer>();
}


inline DiffieHellmanExchange::Integer
DiffieHellmanExchange::mix (Integer const& shared_base,
							Integer const& shared_modulo,
							Integer const& secret)
{
	return powm (shared_base, secret, shared_modulo);
}

} // namespace neutrino


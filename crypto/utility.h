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

#ifndef NEUTRINO__CRYPTO__UTILITY_H__INCLUDED
#define NEUTRINO__CRYPTO__UTILITY_H__INCLUDED

// Standard:
#include <cstddef>

// Boost:
#include <boost/random.hpp>
#include <boost/random/random_device.hpp>
#include <boost/multiprecision/cpp_int.hpp>

// Neutrino:
#include <neutrino/blob.h>


namespace neutrino {

/**
 * Wipe argument's buffer with unspecified data.
 */
inline void
wipe (boost::multiprecision::cpp_int& integer)
{
	auto const bits = integer.backend().size() * sizeof (integer.backend().limbs()[0]);

	for (uint32_t b = 0; b < bits; ++b)
		bit_set (integer, b);
}


/**
 * Reduce longer vector to shorter by xoring it with excess data.
 *
 * \param	target_size
 *			Size of returned vector (bytes).
 */
inline Blob
xor_reduce (Blob const& source, uint32_t target_size)
{
	Blob result (target_size, 0);

	for (std::size_t i = 0; i < source.size(); ++i)
		result[i % target_size] ^= source[i];

	return result;
}


/**
 * Produce a random blob using given random device.
 */
inline Blob
random_blob (size_t const bytes, boost::random::random_device& device)
{
	auto distribution = boost::random::uniform_int_distribution<uint8_t> (0, 255);
	Blob result (bytes, 0x00);

	for (auto& c: result)
		c = distribution (device);

	return result;
}

} // namespace neutrino

#endif


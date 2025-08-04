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

#ifndef NEUTRINO__CORE_TYPES_H__INCLUDED
#define NEUTRINO__CORE_TYPES_H__INCLUDED

// Boost:
#include <boost/align/aligned_allocator.hpp>

// Standard:
#include <cstdint>
#include <memory>
#include <optional>
#include <stdfloat>
#include <string>
#include <vector>


constexpr std::size_t
operator""_bit (unsigned long long bits)
{
	return 1ull << bits;
}


using float16_t		= std::float16_t;
using float32_t		= float;
using float64_t		= double;
using float128_t	= long double;

// C-compatibility:
typedef bool _Bool;

#endif


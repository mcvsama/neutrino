/* vim:ts=4
 *
 * Copyleft 2019  Michał Gawron
 * Marduk Unix Labs, http://mulabs.org/
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Visit http://www.gnu.org/licenses/gpl-3.0.html for more information on licensing.
 * --
 * Here be basic, global functions and macros like asserts, debugging helpers, etc.
 */

#ifndef NEUTRINO__ENDIAN_H__INCLUDED
#define NEUTRINO__ENDIAN_H__INCLUDED

// Standard:
#include <cstring>

// Boost:
#include <boost/endian/conversion.hpp>

// Neutrino:
#include <neutrino/types.h>


namespace neutrino {

/**
 * Convert numeric value endianess.
 * Boost.Endian stopped supporting floats at some point, we assume that IEEE-754 will use little-endian.
 * FIXME This might need to be fixed, also floats might have different endianess than integers.
 */
template<class Value>
	void
	native_to_little (Value& value)
	{
		if constexpr (std::is_integral_v<Value>)
			boost::endian::native_to_little (value);
	}


/**
 * Convert numeric value endianess.
 * Boost.Endian stopped supporting floats at some point, we assume that IEEE-754 will use little-endian.
 * FIXME This might need to be fixed, also floats might have different endianess than integers.
 */
template<class Value>
	void
	little_to_native (Value& value)
	{
		if constexpr (std::is_integral_v<Value>)
			boost::endian::little_to_native (value);
	}

} // namespace neutrino

#endif

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
#include <neutrino/concepts.h>


namespace neutrino {

/**
 * Convert numeric value endianess.
 */
template<class Value>
	void
	perhaps_native_to_little_inplace (Value& value)
	{
		// TODO Use this when Boost is fixed and supports float/double conversions: if constexpr (ArithmeticConcept<Value>)
		if constexpr (std::integral<Value>)
			boost::endian::native_to_little_inplace (value);
	}


/**
 * Convert numeric value endianess.
 */
template<class Value>
	void
	perhaps_little_to_native_inplace (Value& value)
	{
		// TODO Use this when Boost is fixed and supports float/double conversions: if constexpr (ArithmeticConcept<Value>)
		if constexpr (std::integral<Value>)
			boost::endian::little_to_native_inplace (value);
	}

} // namespace neutrino

#endif

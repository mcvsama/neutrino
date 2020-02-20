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


namespace neutrino {

/**
 * Convert floating point endianess.
 * Boost.Endian stopped supporting floats at some point, we have to workaround this.
 */
template<class Value>
	void
	native_to_little (Value& value)
	{
		if constexpr (std::is_floating_point_v<Value>)
		{
			xf::int_for_width_t<sizeof (value)> integer_value;
			std::memcpy (&integer_value, &float_value, sizeof (value));
			boost::native_to_little (integer_value);
			std::memcpy (&float_value, &integer_value, sizeof (value));
		}
		else
			boost::native_to_little (value);
	}


/**
 * Convert floating point endianess.
 * Boost.Endian stopped supporting floats at some point, we have to workaround this.
 */
template<class Value>
	void
	little_to_native (Value& value)
	{
		if constexpr (std::is_floating_point_v<Value>)
		{
			xf::int_for_width_t<sizeof (value)> integer_value;
			std::memcpy (&integer_value, &float_value, sizeof (value));
			boost::little_to_native (integer_value);
			std::memcpy (&float_value, &integer_value, sizeof (value));
		}
		else
			boost::little_to_native (value);
	}

} // namespace exception_ops
} // namespace neutrino

#endif

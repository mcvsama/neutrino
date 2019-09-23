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

#ifndef NEUTRINO__SI__CONVERSION_EXPLICIT_H__INCLUDED
#define NEUTRINO__SI__CONVERSION_EXPLICIT_H__INCLUDED

// Standard:
#include <cstddef>

// Local:
#include "standard_literals.h"


namespace neutrino::si {

/**
 * Explicitly convert from angular (in radians) to base (in Hertz).
 * Works on any type that can be multiplied by a scalar.
 */
constexpr auto
angular_to_base (auto const& value)
{
	using namespace literals;

	return value / (2_rad * M_PI);
}


/**
 * Explicitly convert from base values (Hz) to angular (in radians).
 * Works on any type that can be multiplied by a scalar.
 */
constexpr auto
base_to_angular (auto const& value)
{
	using namespace literals;

	return 2_rad * M_PI * value;
}

} // namespace neutrino::si

#endif


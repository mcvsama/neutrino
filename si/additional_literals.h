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

#ifndef NEUTRINO__SI__ADDITIONAL_LITERALS_H__INCLUDED
#define NEUTRINO__SI__ADDITIONAL_LITERALS_H__INCLUDED

// Local:
#include "quantity.h"
#include "unit.h"

// Standard:
#include <cstddef>
#include <ratio>
#include <utility>


namespace neutrino::si::literals {

[[nodiscard]]
constexpr auto
operator"" _dpi (long double value)
{
	return decltype(1 / Quantity<units::Inch>()) (value);
}


[[nodiscard]]
constexpr auto
operator"" _dpi (unsigned long long value)
{
	return decltype(1 / Quantity<units::Inch>()) (value);
}

} // namespace neutrino::si::literals

#endif


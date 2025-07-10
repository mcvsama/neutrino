/* vim:ts=4
 *
 * Copyleft 2024  Michał Gawron
 * Marduk Unix Labs, http://mulabs.org/
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Visit http://www.gnu.org/licenses/gpl-3.0.html for more information on licensing.
 */

#ifndef NEUTRINO__FORMAT_H__INCLUDED
#define NEUTRINO__FORMAT_H__INCLUDED

// Neutrino:
#include <neutrino/si/concepts.h>
#include <neutrino/si/utils.h>

// Standard:
#include <cstddef>
#include <cstdint>
#include <string>


namespace neutrino {

/**
 * For `divider` use 1024 for KiB, MiB, etc, and 1000 for everything else.
 */
[[nodiscard]]
std::string
format_unit (double value, uint8_t width_excl_dot, std::string_view unit, unsigned int divider = 1000);


/**
 * Overload for SI units.
 */
template<si::QuantityConcept Q>
	[[nodiscard]]
	inline std::string
	format_unit (Q const value, uint8_t width_excl_dot, unsigned int divider = 1000)
	{
		return format_unit (value.base_value(), width_excl_dot, si::unit_symbol<si::Quantity<typename Q::NormalizedUnit>>(), divider);
	}

} // namespace neutrino

#endif


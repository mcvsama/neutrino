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

// Standard:
#include <cstddef>
#include <cstdint>
#include <string>


namespace neutrino {

/**
 * For [divider] use 1024 for KiB, MiB, etc, and 1000 for everything else.
 */
std::string
format_unit (double value, uint8_t width_excl_dot, std::string_view unit, unsigned int divider = 1000);

} // namespace neutrino

#endif


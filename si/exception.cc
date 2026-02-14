/* vim:ts=4
 *
 * Copyleft 2012-2024  Michał Gawron
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
#include "si_config.h"
#include "exception.h"
#include "unit.h"

// Standard:
#include <cstddef>
#include <format>


namespace neutrino::si {

IncompatibleTypes::IncompatibleTypes (DynamicUnit const& got, DynamicUnit const& expected, std::optional<bool> include_backtrace):
	Exception (std::format ("incompatible types; expected '{}', got '{}'", expected.symbol(), got.symbol()), include_backtrace)
{ }

} // namespace neutrino::si

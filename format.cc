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

// Local:
#include "format.h"

// Standard:
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <format>
#include <string>


namespace neutrino {

std::string
format_unit (double const value, uint8_t const width_excl_dot, std::string_view const unit, unsigned int const divider)
{
	static std::array<std::string, 21> metric_unit_prefixes = {
		"q",	// quecto
		"r",	// ronto
		"y",	// yocto
		"z",	// zepto
		"a",	// atto
		"f",	// femto
		"p",	// pico
		"n",	// nano
		"µ",	// micro
		"m",	// milli
		"",		// –
		"k",	// kilo
		"M",	// Mega
		"G",	// Giga
		"T",	// Tera
		"P",	// Peta
		"E",	// Exa
		"Z",	// Zetta
		"Y",	// Yotta
		"R",	// Ronna
		"Q",	// Quetta
	};
	int const neutral_index = 10;
	auto const unit_rank = std::floor (std::log (std::abs (value)) / std::log (divider));
	auto const unit_index_f = unit_rank + neutral_index;

	if (std::isfinite (unit_rank) && unit_index_f > 0.0 && unit_index_f < metric_unit_prefixes.size())
	{
		try {
			auto const unit_index = static_cast<std::size_t> (unit_index_f);
			auto const unit_prefix = metric_unit_prefixes.at (unit_index);
			auto const scaled_value = value / std::pow (divider, unit_rank);
			auto const space = unit.empty() ? "" : " ";
			auto const precision = scaled_value >= 100.0
				? std::max (0, width_excl_dot - 3)
				: scaled_value >= 10.0
					? std::max (0, width_excl_dot - 2)
					: scaled_value >= 1.0
						? std::max (0, width_excl_dot - 1)
						: width_excl_dot;
			auto const format_for_value = std::format ("{{:.{}f}}", precision);
			auto const formatted_value = std::vformat (format_for_value, std::make_format_args (scaled_value));
			return std::format ("{}{}{}{}", formatted_value, space, unit_prefix, unit);
		}
		catch (std::out_of_range&)
		{
			return std::format ("{} {}", value, unit);
		}
	}
	else
		return std::format ("{} {}", value, unit);
}

} // namespace neutrino

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

static std::string
format_unit_with_rank (double const value,
					   uint8_t const width_excl_dot,
					   std::string_view const unit,
					   unsigned int const divider,
					   int const unit_rank,
					   bool const zero_clamped)
{
	static std::array<std::string, 21> const metric_unit_prefixes = {
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
	int constexpr neutral_index = 10;
	int const unit_index = unit_rank + neutral_index;

	if (unit_index <= 0 || unit_index >= static_cast<int> (metric_unit_prefixes.size()))
		return std::format ("0 {}", unit);

	try {
		auto const unit_prefix = metric_unit_prefixes.at (static_cast<std::size_t> (unit_index));
		auto const scaled_value = zero_clamped ? 0.0 : value / std::pow (divider, unit_rank);
		auto const scaled_value_abs = std::abs (scaled_value);
		auto const space = unit.empty() ? "" : " ";
		auto const precision = zero_clamped
			? 0
			: scaled_value_abs >= 100.0
				? std::max (0, width_excl_dot - 3)
				: scaled_value_abs >= 10.0
					? std::max (0, width_excl_dot - 2)
					: scaled_value_abs >= 1.0
						? std::max (0, width_excl_dot - 1)
						: width_excl_dot;
		auto const format_for_value = std::format ("{{:.{}f}}", precision);
		auto const formatted_value = std::vformat (format_for_value, std::make_format_args (scaled_value));
		return std::format ("{}{}{}{}", formatted_value, space, unit_prefix, unit);
	}
	catch (std::out_of_range&)
	{
		return zero_clamped
			? std::format ("0 {}", unit)
			: std::format ("{} {}", value, unit);
	}
}


std::string
format_unit (double const value,
			 uint8_t const width_excl_dot,
			 std::string_view const unit,
			 unsigned int const divider,
			 std::optional<double> const minimum_displayed_value)
{
	if (minimum_displayed_value &&
		std::isfinite (*minimum_displayed_value) &&
		*minimum_displayed_value > 0.0 &&
		std::abs (value) < *minimum_displayed_value)
	{
		auto const minimum_value_rank = std::floor (std::log (std::abs (*minimum_displayed_value)) / std::log (divider));

		if (std::isfinite (minimum_value_rank))
			return format_unit_with_rank (value, width_excl_dot, unit, divider, minimum_value_rank, true);
	}

	auto const unit_rank = std::floor (std::log (std::abs (value)) / std::log (divider));

	if (std::isfinite (unit_rank))
		return format_unit_with_rank (value, width_excl_dot, unit, divider, unit_rank, false);
	else
		return std::format ("{} {}", value, unit);
}

} // namespace neutrino

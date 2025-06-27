/* vim:ts=4
 *
 * Copyleft 2025  Michał Gawron
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
#include "time_helper.h"

// Standard:
#include <cstddef>


namespace neutrino {

si::Time
TimeHelper::utc_now() noexcept
{
	static_assert (std::ratio_less_equal_v<std::chrono::utc_clock::period, std::micro>);

	using namespace si::literals;
	auto const t = std::chrono::utc_clock::now();
	auto const t_us = std::chrono::time_point_cast<std::chrono::microseconds> (t);
	return 1_us * t_us.time_since_epoch().count();
}


si::Time
TimeHelper::system_now() noexcept
{
	static_assert (std::ratio_less_equal_v<std::chrono::system_clock::period, std::micro>);

	using namespace si::literals;
	auto const t = std::chrono::system_clock::now();
	auto const t_us = std::chrono::time_point_cast<std::chrono::microseconds> (t);
	return 1_us * t_us.time_since_epoch().count();
}

} // namespace neutrino


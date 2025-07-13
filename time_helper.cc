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
namespace {

template<class Clock>
	[[nodiscard]]
	si::Time
	any_now()
	{
		static_assert (std::ratio_less_equal_v<typename Clock::period, std::nano>);
		using namespace si::literals;
		auto const t = Clock::now();
		auto const t_us = std::chrono::time_point_cast<std::chrono::nanoseconds> (t);
		return 1_ns * t_us.time_since_epoch().count();
	}

} // namespace


si::Time
utc_now() noexcept
{
	return any_now<std::chrono::utc_clock>();
}


si::Time
system_now() noexcept
{
	return any_now<std::chrono::system_clock>();
}


si::Time
steady_now() noexcept
{
	return any_now<std::chrono::steady_clock>();
}

} // namespace neutrino


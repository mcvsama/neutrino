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

#ifndef NEUTRINO__TIME_HELPER_H__INCLUDED
#define NEUTRINO__TIME_HELPER_H__INCLUDED

// Neutrino:
#include <neutrino/si/si.h>

// System:
#include <sys/time.h>

// Standard:
#include <chrono>
#include <cstddef>
#include <ratio>
#include <type_traits>


namespace neutrino {

class TimeHelper
{
  public:
	static si::Time
	now() noexcept;

	static si::Time
	epoch() noexcept;

	template<class Callable>
		static si::Time
		measure (Callable&& callback);
};


inline si::Time
TimeHelper::now() noexcept
{
	using namespace si::literals;

	static_assert (std::ratio_less_equal_v<std::chrono::system_clock::period, std::micro>);

	auto const t = std::chrono::system_clock::now();
	auto const t_us = std::chrono::time_point_cast<std::chrono::microseconds> (t);
	return 1_us * t_us.time_since_epoch().count();
}


inline si::Time
TimeHelper::epoch() noexcept
{
	using namespace si::literals;

	return 0_s;
}


template<class Callable>
	inline si::Time
	TimeHelper::measure (Callable&& callback)
	{
		si::Time t = now();
		callback();
		return now() - t;
	}

} // namespace neutrino

#endif


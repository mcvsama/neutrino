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
	/**
	 * Return UTC 'now'.
	 */
	static si::Time
	utc_now() noexcept;

	/**
	 * Return system's clock 'now'.
	 */
	static si::Time
	system_now() noexcept;

	static si::Time
	epoch() noexcept;

	template<class Callable>
		static si::Time
		measure (Callable&& callback);
};


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
		si::Time t = system_now();
		callback();
		return system_now() - t;
	}

} // namespace neutrino

#endif


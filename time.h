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

#ifndef NEUTRINO__TIME_H__INCLUDED
#define NEUTRINO__TIME_H__INCLUDED

// Neutrino:
#include <neutrino/si/si.h>

// System:
#include <time.h>

// Standard:
#include <chrono>
#include <cstddef>
#include <ratio>
#include <type_traits>


namespace neutrino {

// TODO Make Timestamp absolute and create operators for dealing with si::Time
using Timestamp = si::Time;


void
sleep (si::Time time);


/**
 * Return UTC 'now'.
 */
[[nodiscard]]
si::Time
utc_now() noexcept;


/**
 * Return system's clock 'now'.
 */
[[nodiscard]]
si::Time
system_now() noexcept;


/**
 * Return steady clock's 'now'.
 */
[[nodiscard]]
si::Time
steady_now() noexcept;


/**
 * Return time that represents Epoch timepoint.
 */
[[nodiscard]]
inline si::Time
epoch() noexcept
{
	using namespace si::literals;
	return 0_s;
}


/**
 * Measure time it takes to execute callback using steady_clock.
 */
[[nodiscard]]
si::Time
measure_time (std::invocable auto&& callback)
{
	si::Time const t0 = steady_now();
	callback();
	return steady_now() - t0;
}

} // namespace neutrino

#endif

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
#include "time.h"

// Standard:
#include <cstddef>


namespace neutrino {

void
sleep (si::Time const time)
{
	using namespace si::literals;

	struct timespec ts;
	ts.tv_sec = static_cast<decltype (ts.tv_sec)> (time.in<si::Second>());
	ts.tv_nsec = (time - ts.tv_sec * 1_s).in<si::Nanosecond>();

	do {
		if (nanosleep (&ts, &ts) == -1)
			if (errno == EINTR)
				continue;
	} while (false);
}

} // namespace neutrino


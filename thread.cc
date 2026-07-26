/* vim:ts=4
 *
 * Copyleft 2008…2013  Michał Gawron
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
#include "thread.h"

// System:
#include <pthread.h>

// Standard:
#include <cstddef>


namespace neutrino {

static void
set_native_handle (std::thread::native_handle_type native_handle, ThreadScheduler scheduler, int priority)
{
	struct sched_param p;
	memset (&p, 0, sizeof (p));
	p.sched_priority = priority;

	switch (::pthread_setschedparam (native_handle, static_cast<int> (scheduler), &p))
	{
		case ESRCH:		throw SchedulerException ("specified thread not found");
		case EINVAL:	throw SchedulerException ("unrecognized scheduling policy or invalid param for the policy");
		case EPERM:		throw SchedulerException ("permission denied for setting thread scheduling policy");
	}
}


void
set (std::thread& thread, ThreadScheduler scheduler, int priority)
{
	if (thread.joinable())
		set_native_handle (thread.native_handle(), scheduler, priority);
}


void
set (std::jthread& thread, ThreadScheduler scheduler, int priority)
{
	if (thread.joinable())
		set_native_handle (thread.native_handle(), scheduler, priority);
}

} // namespace neutrino

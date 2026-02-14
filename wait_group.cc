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
#include "wait_group.h"

// Standard:
#include <cstddef>


namespace neutrino {

void
WaitGroup::WorkToken::done()
{
	if (_group)
	{
		_group->done();
		_group = nullptr;
	}
}


void
WaitGroup::add (std::size_t const count)
{
	auto lock = std::lock_guard (_mutex);
	_counter += count;
}


void
WaitGroup::done()
{
	auto lock = std::lock_guard (_mutex);

	if (_counter == 0)
		throw PreconditionFailed ("WaitGroup: more calls to done() than add()");

	--_counter;

	if (_counter == 0)
		_condition.notify_all();
}


void
WaitGroup::wait()
{
	auto lock = std::unique_lock (_mutex);
	_condition.wait (lock, [this] { return _counter == 0; });
}

} // namespace neutrino

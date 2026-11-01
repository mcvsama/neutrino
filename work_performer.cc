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
#include "work_performer.h"

// Neutrino:
#include <neutrino/thread.h>

// Standard:
#include <cstddef>
#include <format>
#include <utility>


namespace neutrino {

WorkPerformer::WorkPerformer (std::size_t threads_number, Logger const& logger):
	_logger (logger.with_context ("<work performer>"))
{
	if (threads_number == 0)
		threads_number = 1;

	_threads.reserve (threads_number);

	try {
		for (std::size_t i = 0; i < threads_number; ++i)
			_threads.emplace_back (&WorkPerformer::thread, this);
	}
	catch (...)
	{
		stop_and_join();
		throw;
	}
}


WorkPerformer::~WorkPerformer()
{
	auto const abandoned_tasks = stop_and_join();

	try {
		if (abandoned_tasks > 0)
			_logger << std::format ("Destroyed WorkPerformer with {} tasks never started\n", abandoned_tasks);
	}
	catch (...)
	{
		detail::report_logging_failure();
	}
}


void
WorkPerformer::set (ThreadScheduler scheduler, int priority)
{
	for (auto& thread: _threads)
		neutrino::set (thread, scheduler, priority);
}


std::size_t
WorkPerformer::queued_tasks() const noexcept
{
	return _state.lock()->tasks.size();
}


std::size_t
WorkPerformer::stop_and_join()
{
	{
		auto state = _state.lock();
		state->terminating = true;
	}

	std::size_t abandoned_tasks = 0;

	while (true)
	{
		std::unique_ptr<AbstractTask> task;
		{
			auto state = _state.lock();

			if (state->tasks.empty())
				break;

			task = std::move (state->tasks.front());
			state->tasks.pop();
		}
		++abandoned_tasks;
		// The task is destroyed here, outside the lock, before any worker is joined.
	}

	_tasks_condition.notify_all();

	for (auto& thread: _threads)
		thread.join();

	return abandoned_tasks;
}


void
WorkPerformer::thread()
{
	while (true)
	{
		std::unique_ptr<AbstractTask> task;

		{
			auto state = _state.lock();
			auto const ready = [&state] { return state->terminating || !state->tasks.empty(); };
			state.wait (_tasks_condition, ready);

			if (state->terminating)
				return;

			task = std::move (state->tasks.front());
			state->tasks.pop();
		}

		// It's guaranteed now that task is not null:
		(*task)();
	}
}

} // namespace neutrino

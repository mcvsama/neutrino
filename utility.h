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

#ifndef NEUTRINO__UTILITY_H__INCLUDED
#define NEUTRINO__UTILITY_H__INCLUDED

// Neutrino:
#include <neutrino/scope_exit.h>

// Standard:
#include <cstddef>
#include <future>
#include <optional>
#include <type_traits>


namespace neutrino {

template<class Value, class ...Rest>
	Value*
	coalesce (Value* value, Rest* ...rest)
	{
		if (value)
			return value;
		else if (sizeof...(rest) > 0)
			return coalesce (rest...);
		else
			return nullptr;
	}


/**
 * Create a copy of an argument.
 */
template<class T>
	inline T
	clone (T t)
	{
		return T (t);
	}


/**
 * Return true if a std::future is ready.
 * Doesn't check if future is valid - sometimes you want to separately test if a future
 * is valid but not ready.
 */
template<class Result>
	inline bool
	ready (std::future<Result> const& future)
	{
		return future.wait_for (std::chrono::seconds (0)) == std::future_status::ready;
	}


/**
 * Return true if a std::shared_future is ready.
 * Doesn't check if future is valid - sometimes you want to separately test if a future
 * is valid but not ready.
 */
template<class Result>
	inline bool
	ready (std::shared_future<Result> const& future)
	{
		return future.wait_for (std::chrono::seconds (0)) == std::future_status::ready;
	}


/**
 * Return true if a std::future is valid and ready.
 */
template<class Result>
	inline bool
	valid_and_ready (std::future<Result> const& future)
	{
		return future.valid() && ready (future);
	}


/**
 * Return true if a std::shared_future is valid and ready.
 */
template<class Result>
	inline bool
	valid_and_ready (std::shared_future<Result> const& future)
	{
		return future.valid() && ready (future);
	}


/**
 * Attempt to acquire a simple boolean lock and return a `ScopeExit` guard to release it.
 *
 * This function checks the provided boolean flag `lock_variable`. If the flag is already
 * true, it means the lock is held elsewhere, so the function returns disengaged ScopeExit
 * to indicate failure to acquire the lock. If the flag is false, the function sets it
 * to true (acquiring the lock) and returns an engaged `ScopeExit<>` object that, upon
 * destruction, will reset the flag back to false (releasing the lock).
 *
 * Usage:
 *   if (auto guard = widget.bool_lock (my_flag))
 *   {
 *       // Lock acquired; do critical work here
 *   }
 *
 * \param	lock_variable  Reference to the boolean flag acting as the lock.
 * \return	`ScopeExit`
 *			- Engaged `ScopeExit` if lock was successfully acquired.
 *			- Disengaged `ScopeExit` if the lock was already held.
 */
inline auto
bool_lock (bool& lock_variable)
{
	auto unlock = [&lock_variable] { lock_variable = false; };

	if (lock_variable)
		return ScopeExit<decltype (unlock)>();
	else
	{
		lock_variable = true;
		return ScopeExit (unlock);
	}
}

} // namespace neutrino

#endif

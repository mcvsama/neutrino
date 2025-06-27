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

#ifndef NEUTRINO__SCOPE_EXIT_H__INCLUDED
#define NEUTRINO__SCOPE_EXIT_H__INCLUDED

// Standard:
#include <cstddef>
#include <functional>


namespace neutrino {

/**
 * Define an object which executes given function upon destruction.
 * Useful substitute for "finally" construct, nonexistent in C++.
 */
class ScopeExit
{
  public:
	typedef std::function<void()> Callback;

  public:
	// Ctor
	ScopeExit() noexcept = default;

	// Ctor
	explicit
	ScopeExit (Callback callback) noexcept;

	// Copy ctor
	ScopeExit (ScopeExit const&) = delete;

	// Move ctor
	ScopeExit (ScopeExit&& other) noexcept;

	// Copy operator
	ScopeExit&
	operator= (ScopeExit const&) = delete;

	// Move operator
	ScopeExit&
	operator= (ScopeExit&& other) noexcept;

	/**
	 * Assign new responsibility. Release previous one.
	 */
	ScopeExit&
	operator= (Callback callback) noexcept;

	/**
	 * Releases resouce (calls the callback).
	 */
	~ScopeExit();

	/**
	 * Execute the tracked responsibility.
	 */
	void
	execute();

	/**
	 * Release the responsibility so that it's not tracked anymore.
	 */
	void
	release();

  private:
	Callback _callback;
};


inline
ScopeExit::ScopeExit (Callback callback) noexcept:
	_callback (callback)
{ }


inline
ScopeExit::ScopeExit (ScopeExit&& other) noexcept:
	_callback (other._callback)
{
	other.release();
}


inline ScopeExit&
ScopeExit::operator= (ScopeExit&& other) noexcept
{
	execute();
	_callback = other._callback;
	other.release();
	return *this;
}


inline ScopeExit&
ScopeExit::operator= (Callback callback) noexcept
{
	execute();
	_callback = callback;
	return *this;
}


inline
ScopeExit::~ScopeExit()
{
	execute();
}


inline void
ScopeExit::execute()
{
	if (_callback)
		_callback();
	release();
}


inline void
ScopeExit::release()
{
	_callback = nullptr;
}

} // namespace neutrino

#endif


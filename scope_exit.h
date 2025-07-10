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
#include <concepts>
#include <cstddef>
#include <optional>


namespace neutrino {

/**
 * Define an object which executes given function upon destruction.
 * Useful substitute for "finally" construct, nonexistent in C++.
 */
template<std::invocable tCallback = std::function<void()>>
	class ScopeExit
	{
	  public:
		using Callback = tCallback;

	  public:
		// Ctor
		ScopeExit() noexcept = default;

		// Ctor
		explicit
		ScopeExit (Callback& callback) noexcept;

		explicit
		ScopeExit (Callback&& callback) noexcept;

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
		~ScopeExit()
			{ this->operator()(); }

		/**
		 * Execute the tracked responsibility function.
		 */
		void
		operator()();

		/**
		 * Return true if the ScopeExit is engaged (has code to execute upon destruction).
		 */
		operator bool() const
			{ return _callback.has_value(); }

		/**
		 * Release the responsibility so that it's not tracked anymore.
		 */
		void
		release()
			{ _callback.reset(); }

	  private:
		std::optional<Callback> _callback;
	};


template<std::invocable C>
	inline
	ScopeExit<C>::ScopeExit (Callback& callback) noexcept:
		_callback (callback)
	{ }


template<std::invocable C>
	inline
	ScopeExit<C>::ScopeExit (Callback&& callback) noexcept:
		_callback (std::move (callback))
	{ }


template<std::invocable C>
	inline
	ScopeExit<C>::ScopeExit (ScopeExit&& other) noexcept:
		_callback (other._callback)
	{
		other.release();
	}


template<std::invocable C>
	inline ScopeExit<C>&
	ScopeExit<C>::operator= (ScopeExit&& other) noexcept
	{
		this->operator()();
		_callback = other._callback;
		other.release();
		return *this;
	}


template<std::invocable C>
	inline ScopeExit<C>&
	ScopeExit<C>::operator= (Callback callback) noexcept
	{
		this->operator()();
		_callback = callback;
		return *this;
	}


template<std::invocable C>
	inline void
	ScopeExit<C>::operator()()
	{
		if (_callback)
			(*_callback)();

		release();
	}

} // namespace neutrino

#endif


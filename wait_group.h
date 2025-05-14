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

#ifndef NEUTRINO__WAIT_GROUP_H__INCLUDED
#define NEUTRINO__WAIT_GROUP_H__INCLUDED

// Neutrino:
#include <neutrino/stdexcept.h>

// Standard:
#include <atomic>
#include <condition_variable>
#include <mutex>


namespace neutrino {

class WaitGroup
{
  public:
	/**
	 * This token, when created, adds 1 to the WaitGroup.
	 * When destroyed, calls done() on the WaitGroup.
	 */
	class WorkToken
	{
	  public:
		// Ctor
		WorkToken (WaitGroup& group);

		// Copy ctor
		WorkToken (WorkToken const&) = delete;

		// Move ctor
		WorkToken (WorkToken&&);

		// Copy operator
		WorkToken&
		operator= (WorkToken const&) = delete;

		// Move operator
		WorkToken&
		operator= (WorkToken&&) = delete;

		// Dtor
		~WorkToken();

		void
		done();

	  private:
		WaitGroup* _group;
	};

  public:
	void
	add (std::size_t count = 1);

	void
	done();

	/**
	 * Wait until the counter reaches 0.
	 */
	void
	wait();

	WorkToken
	make_work_token();

  private:
	std::mutex				_mutex;
	std::condition_variable	_condition;
	std::size_t				_counter { 0 };
};


inline
WaitGroup::WorkToken::WorkToken (WaitGroup& group):
	_group (&group)
{
	_group->add();
}


inline
WaitGroup::WorkToken::WorkToken (WorkToken&& other):
	_group (other._group)
{
	other._group = nullptr;
}


inline
WaitGroup::WorkToken::~WorkToken()
{
	done();
}


inline void
WaitGroup::WorkToken::done()
{
	if (_group)
	{
		_group->done();
		_group = nullptr;
	}
}


inline void
WaitGroup::add (std::size_t const count)
{
	auto lock = std::lock_guard (_mutex);
	_counter += count;
}


inline void
WaitGroup::done()
{
	auto lock = std::lock_guard (_mutex);

	if (_counter == 0)
		throw PreconditionFailed ("WaitGroup: more calls to done() than add()");

	--_counter;

	if (_counter == 0)
		_condition.notify_all();
}


inline void
WaitGroup::wait()
{
	auto lock = std::unique_lock (_mutex);
	_condition.wait (lock, [this] { return _counter == 0; });
}


inline WaitGroup::WorkToken
WaitGroup::make_work_token()
{
	return WorkToken (*this);
}

} // namespace neutrino

#endif


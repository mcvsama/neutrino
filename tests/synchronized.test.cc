/* vim:ts=4
 *
 * Copyleft 2026  Michał Gawron
 * Marduk Unix Labs, http://mulabs.org/
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Visit http://www.gnu.org/licenses/gpl-3.0.html for more information on licensing.
 */

// Neutrino:
#include <neutrino/synchronized.h>

// Standard:
#include <memory>
#include <mutex>
#include <type_traits>


namespace neutrino::test {
namespace {

struct CopyOnly
{
	CopyOnly() = default;
	CopyOnly (CopyOnly const&) = default;
	CopyOnly (CopyOnly&&) = delete;
	CopyOnly& operator= (CopyOnly const&) = default;
	CopyOnly& operator= (CopyOnly&&) = delete;
};


struct CopyAssignOnly
{
	CopyAssignOnly() = default;
	CopyAssignOnly (CopyAssignOnly const&) = delete;
	CopyAssignOnly& operator= (CopyAssignOnly const&) = default;
};


struct BasicMutex
{
	std::mutex mutex;

	void
	lock()
		{ this->mutex.lock(); }

	void
	unlock()
		{ this->mutex.unlock(); }
};


struct NonDefaultMutex: std::mutex
{
	NonDefaultMutex() = delete;
};


struct InvalidTryLockMutex: BasicMutex
{
	void
	try_lock()
		{ }
};


class SelfBorrow
{
  public:
	SelfBorrow() = default;

	void
	update() const
		{ _mutable_this->value = 2; }

	int value { 1 };

  private:
	Synchronized<SelfBorrow&> mutable _mutable_this { *this };
};


using MoveOnly = Synchronized<std::unique_ptr<int>>;

static_assert (!std::is_copy_constructible_v<MoveOnly>);
static_assert (!std::is_copy_assignable_v<MoveOnly>);
static_assert (!std::is_constructible_v<MoveOnly, MoveOnly&>);

static_assert (std::is_copy_constructible_v<Synchronized<int>>);
static_assert (std::is_copy_assignable_v<Synchronized<int>>);
static_assert (std::is_constructible_v<Synchronized<int>, Synchronized<int>&>);
static_assert (!std::is_move_constructible_v<Synchronized<int>>);
static_assert (!std::is_move_assignable_v<Synchronized<int>>);

static_assert (std::is_copy_constructible_v<Synchronized<int const>>);
static_assert (!std::is_copy_assignable_v<Synchronized<int const>>);
static_assert (!std::is_copy_constructible_v<Synchronized<int&>>);
static_assert (!std::is_copy_assignable_v<Synchronized<int&>>);
static_assert (!std::is_constructible_v<Synchronized<int&>, Synchronized<int&>&>);
static_assert (!std::is_copy_constructible_v<Synchronized<int const&>>);
static_assert (!std::is_copy_assignable_v<Synchronized<int const&>>);
static_assert (!std::is_constructible_v<Synchronized<int const&>, Synchronized<int const&>&>);
static_assert (std::is_constructible_v<Synchronized<int&>, int&>);
static_assert (std::is_constructible_v<Synchronized<int const&>, int const&>);

static_assert (std::is_default_constructible_v<SelfBorrow>);
static_assert (!std::is_copy_constructible_v<SelfBorrow>);
static_assert (!std::is_copy_assignable_v<SelfBorrow>);
static_assert (std::is_constructible_v<Synchronized<SelfBorrow&>, SelfBorrow&>);
static_assert (!std::is_copy_constructible_v<Synchronized<SelfBorrow&>>);
static_assert (!std::is_copy_assignable_v<Synchronized<SelfBorrow&>>);

static_assert (std::is_copy_constructible_v<Synchronized<CopyOnly>>);
static_assert (std::is_copy_assignable_v<Synchronized<CopyOnly>>);
static_assert (!std::is_copy_constructible_v<Synchronized<CopyAssignOnly>>);
static_assert (std::is_copy_assignable_v<Synchronized<CopyAssignOnly>>);

static_assert (std::is_copy_constructible_v<Synchronized<int, BasicMutex>>);
static_assert (!std::is_copy_assignable_v<Synchronized<int, BasicMutex>>);
static_assert (!std::is_copy_constructible_v<Synchronized<int, NonDefaultMutex>>);
static_assert (!std::is_copy_assignable_v<Synchronized<int, InvalidTryLockMutex>>);

} // namespace
} // namespace neutrino::test

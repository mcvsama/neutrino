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
#include <neutrino/test/test_asserts.h>
#include <neutrino/test/test_suite.h>

// Standard:
#include <any>
#include <cstddef>
#include <memory>
#include <mutex>
#include <string>
#include <type_traits>
#include <utility>


namespace neutrino::test {
namespace {

struct CopyOnly
{
	CopyOnly() = default;
	CopyOnly (CopyOnly const&) = default;
	CopyOnly (CopyOnly&&) = delete;
	CopyOnly& operator= (CopyOnly const&) = default;
	CopyOnly& operator= (CopyOnly&&) = delete;

	int value { 1 };
};


struct CopyAssignOnly
{
	CopyAssignOnly() = default;
	CopyAssignOnly (CopyAssignOnly const&) = delete;
	CopyAssignOnly& operator= (CopyAssignOnly const&) = default;

	int value { 1 };
};


struct ExplicitCopy
{
	ExplicitCopy() = default;
	explicit ExplicitCopy (ExplicitCopy const&) = default;
	ExplicitCopy& operator= (ExplicitCopy const&) = default;
};


struct MutableCopy
{
	MutableCopy() = default;
	MutableCopy (MutableCopy&) = default;
	MutableCopy (MutableCopy&&) = default;
	MutableCopy& operator= (MutableCopy const&) = default;

	int value { 1 };
};


struct MutableCopyExplicitMove
{
	MutableCopyExplicitMove() = default;
	MutableCopyExplicitMove (MutableCopyExplicitMove&) = default;
	explicit MutableCopyExplicitMove (MutableCopyExplicitMove&&) = default;
	MutableCopyExplicitMove& operator= (MutableCopyExplicitMove const&) = default;
};


struct DerivedSynchronized: Synchronized<std::any>
{
	using Synchronized<std::any>::Synchronized;
};


struct IncompleteArgument;

struct AcceptsIncompleteArgument
{
	explicit AcceptsIncompleteArgument (IncompleteArgument&)
	{ }
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

template<class S>
	concept Loadable = requires (S const& synchronized) {
		synchronized.load();
	};

template<class S>
	concept Storable = requires (S& synchronized, typename S::NonRefValue const& desired) {
		synchronized.store (desired);
	};

template<class S>
	concept Exchangeable = requires (S& synchronized, typename S::NonRefValue const& desired) {
		synchronized.exchange (desired);
	};

static_assert (!std::is_copy_constructible_v<MoveOnly>);
static_assert (!std::is_copy_assignable_v<MoveOnly>);
static_assert (!std::is_constructible_v<MoveOnly, MoveOnly&>);

static_assert (std::is_copy_constructible_v<Synchronized<int>>);
static_assert (std::is_copy_assignable_v<Synchronized<int>>);
static_assert (std::is_default_constructible_v<Synchronized<int>>);
static_assert (std::is_constructible_v<Synchronized<int>, int>);
static_assert (std::is_constructible_v<Synchronized<int>, Synchronized<int>&>);
static_assert (!std::is_move_constructible_v<Synchronized<int>>);
static_assert (!std::is_move_assignable_v<Synchronized<int>>);

static_assert (std::is_constructible_v<Synchronized<std::any>, DerivedSynchronized&>);
static_assert (std::is_constructible_v<Synchronized<std::any>, DerivedSynchronized const&>);
static_assert (!std::is_constructible_v<Synchronized<std::any>, DerivedSynchronized&&>);
static_assert (std::is_constructible_v<Synchronized<std::any>, DerivedSynchronized const&&>);
static_assert (std::is_constructible_v<Synchronized<AcceptsIncompleteArgument>, IncompleteArgument&>);

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
static_assert (!std::is_default_constructible_v<Synchronized<int, NonDefaultMutex>>);
static_assert (!std::is_constructible_v<Synchronized<int, NonDefaultMutex>, int>);
static_assert (!std::is_constructible_v<Synchronized<std::string, NonDefaultMutex>, std::size_t, char>);
static_assert (!std::is_constructible_v<Synchronized<int&, NonDefaultMutex>, int&>);
static_assert (!std::is_constructible_v<Synchronized<int const&, NonDefaultMutex>, int const&>);
static_assert (!std::is_copy_constructible_v<Synchronized<int, NonDefaultMutex>>);
static_assert (!std::is_copy_assignable_v<Synchronized<int, InvalidTryLockMutex>>);

static_assert (Loadable<Synchronized<int>>);
static_assert (Storable<Synchronized<int>>);
static_assert (Exchangeable<Synchronized<int>>);
static_assert (Loadable<Synchronized<int&>>);
static_assert (Storable<Synchronized<int&>>);
static_assert (Exchangeable<Synchronized<int&>>);
static_assert (Loadable<Synchronized<int const>>);
static_assert (!Storable<Synchronized<int const>>);
static_assert (!Exchangeable<Synchronized<int const>>);
static_assert (Loadable<Synchronized<int const&>>);
static_assert (!Storable<Synchronized<int const&>>);
static_assert (!Exchangeable<Synchronized<int const&>>);
static_assert (!Loadable<MoveOnly>);
static_assert (!Storable<MoveOnly>);
static_assert (!Exchangeable<MoveOnly>);
static_assert (Loadable<Synchronized<CopyOnly>>);
static_assert (Storable<Synchronized<CopyOnly>>);
static_assert (Exchangeable<Synchronized<CopyOnly>>);
static_assert (!Loadable<Synchronized<CopyAssignOnly>>);
static_assert (Storable<Synchronized<CopyAssignOnly>>);
static_assert (!Exchangeable<Synchronized<CopyAssignOnly>>);
static_assert (!Loadable<Synchronized<ExplicitCopy>>);
static_assert (Storable<Synchronized<ExplicitCopy>>);
static_assert (!Exchangeable<Synchronized<ExplicitCopy>>);
static_assert (!Loadable<Synchronized<MutableCopy>>);
static_assert (Storable<Synchronized<MutableCopy>>);
static_assert (Exchangeable<Synchronized<MutableCopy>>);
static_assert (!Loadable<Synchronized<MutableCopyExplicitMove>>);
static_assert (Storable<Synchronized<MutableCopyExplicitMove>>);
static_assert (!Exchangeable<Synchronized<MutableCopyExplicitMove>>);


static TestRegistration tests ([]{
	describe ("Neutrino", "Synchronized", []{
		it ("copies the protected value from derived wrappers", []{
			auto verify_copy = [](auto& source) {
				Synchronized<std::any> copy (source);
				auto value = copy.load();
				auto const* integer = std::any_cast<int> (&value);
				test_asserts::verify ("copy contains the protected int", integer != nullptr);
				test_asserts::verify_equal ("copy preserves the protected value", *integer, 42);
			};

			DerivedSynchronized source (42);
			verify_copy (source);
			verify_copy (std::as_const (source));
		});

		it ("forwards multiple arguments that include a derived wrapper", []{
			DerivedSynchronized source (42);
			Synchronized<std::any> synchronized (std::in_place_type<DerivedSynchronized>, source);
			auto value = synchronized.load();
			test_asserts::verify ("explicit in-place construction stores the derived wrapper",
								  std::any_cast<DerivedSynchronized> (&value) != nullptr);
		});

		it ("exchanges values with an explicitly deleted move constructor", []{
			Synchronized<CopyOnly> synchronized;
			CopyOnly desired;
			desired.value = 2;

			auto old = synchronized.exchange (desired);
			test_asserts::verify_equal ("exchange returns the previous value", old.value, 1);
			test_asserts::verify_equal ("exchange stores the desired value", synchronized.load().value, 2);
		});

		it ("exchanges movable values", []{
			Synchronized<std::string> synchronized ("old");

			auto old = synchronized.exchange ("new");
			test_asserts::verify_equal ("exchange returns the previous value", old, "old");
			test_asserts::verify_equal ("exchange stores the desired value", synchronized.load(), "new");
		});

		it ("exchanges values that can only be copied from mutable references", []{
			Synchronized<MutableCopy> synchronized;
			MutableCopy desired;
			desired.value = 2;

			auto old = synchronized.exchange (desired);
			test_asserts::verify_equal ("exchange returns the previous value", old.value, 1);
			test_asserts::verify_equal ("exchange stores the desired value", synchronized.lock()->value, 2);
		});

		it ("stores values without requiring copy construction", []{
			Synchronized<CopyAssignOnly> synchronized;
			CopyAssignOnly desired;
			desired.value = 2;

			synchronized.store (desired);
			test_asserts::verify_equal ("store assigns the desired value", synchronized.lock()->value, 2);
		});
	});
});

} // namespace
} // namespace neutrino::test

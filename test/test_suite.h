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

#ifndef NEUTRINO__TEST__TEST_SUITE_H__INCLUDED
#define NEUTRINO__TEST__TEST_SUITE_H__INCLUDED

// Local:
#include "stdexcept.h"

// Standard:
#include <array>
#include <concepts>
#include <cstddef>
#include <functional>
#include <iosfwd>
#include <list>
#include <memory>
#include <sstream>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>


namespace neutrino {

class TestSuite;


/**
 * Context passed to test callbacks that need to report diagnostics or mark the
 * test as failed without throwing.
 */
class Test
{
  public:
	void
	set_passing (bool passing)
		{ _passing = passing; }

	[[nodiscard]]
	bool
	passing() const noexcept
		{ return _passing; }

  public:
	std::ostringstream cout;

  private:
	bool _passing { true };
};


/**
 * A group of tests and nested test groups.
 *
 * TestSuite stores test definitions during static registration and executes
 * them later, preserving describe()/it() nesting in the printed report.
 */
class TestSuite
{
  public:
	using DescribeCallback1	= std::function<void()>;
	using DescribeCallback2	= std::function<void (TestSuite&)>;
	using DescribeCallback	= std::variant<DescribeCallback1, DescribeCallback2>;
	using DescribeCallbacks	= std::list<DescribeCallback>;

	using TestCallback1		= std::function<void()>;
	using TestCallback2		= std::function<void (Test&)>;
	using TestCallback		= std::variant<TestCallback1, TestCallback2>;
	using TestCallbacks		= std::list<TestCallback>;

  private:
	struct DescribeStep
	{
		std::unique_ptr<TestSuite> test_suite;
	};

	struct ItStep
	{
		std::string		expected_behavior;
		TestCallback	callback;
	};

	using Step = std::variant<DescribeStep, ItStep>;

  public:
	// Ctor
	explicit
	TestSuite (std::string_view name);

	/**
	 * Add a nested test suite.
	 */
	void
	describe (std::string_view tested_thing, DescribeCallback1);

	/**
	 * Add a nested test suite.
	 */
	void
	describe (std::string_view tested_thing, DescribeCallback2);

	/**
	 * Add a nested path of test suites in one call.
	 *
	 * All arguments except the last one form the describe() path, while the last
	 * argument is the leaf callback registered in the final suite.
	 */
	template<class ...Args>
		requires (sizeof...(Args) >= 3)
		inline void
		describe (Args&& ...args);

	/**
	 * Add a test case.
	 */
	void
	it (std::string_view expected_behavior, TestCallback1);

	/**
	 * Add a test case.
	 */
	void
	it (std::string_view expected_behavior, TestCallback2);

	/**
	 * Execute tests and write progress to std::cout.
	 */
	[[nodiscard]]
	bool
	execute() const;

	/**
	 * Execute tests and write progress to the given stream.
	 */
	[[nodiscard]]
	bool
	execute (std::ostream&) const;

	/**
	 * Return parent test suite.
	 */
	TestSuite*
	parent() noexcept
		{ return _parent; }

	/**
	 * Return parent test suite.
	 */
	TestSuite const*
	parent() const noexcept
		{ return _parent; }

 private:
	/**
	 * Create a suite optionally attached to a parent.
	 *
	 * Hidden suites participate in execution and aggregation, but skip printing
	 * their own describe()/summary lines. This is used by the default root suite,
	 * which should execute its children without adding an artificial top-level
	 * wrapper to the report.
	 */
	explicit
	TestSuite (std::string_view name, TestSuite* parent, bool hidden);

	/**
	 * Temporarily make this suite the target for global describe()/it() calls.
	 */
	void
	make_active_for (DescribeCallback1 callback);

	/**
	 * Return an existing direct child suite with the given name, or create one.
	 *
	 * This merges repeated describe() registrations at the same full path into a
	 * single child suite, so their nested steps execute under one shared group.
	 */
	TestSuite&
	get_or_add_child_suite (std::string_view name);

	/**
	 * Execute this suite and all nested steps using the requested indentation
	 * level for its own describe()/summary lines.
	 */
	[[nodiscard]]
	bool
	execute (std::ostream&, uint32_t indent) const;

	/**
	 * Execute a single it() step and print its result at the requested
	 * indentation level.
	 */
	[[nodiscard]]
	static bool
	execute_it_step (ItStep const&, std::ostream&, uint32_t indent);

	[[nodiscard]]
	static uint32_t
	compute_indent (TestSuite* parent_test_suite);

  private:
	TestSuite*						_parent		{ nullptr };
	bool							_hidden		{ false };
	std::string						_name;
	std::list<Step>					_steps;
	std::list<std::exception_ptr>	_registration_exceptions;

  private:
	friend TestSuite&
	default_test_suite();
};


/**
 * Register tests during static initialization.
 *
 * The callback constructor is meant for describe()/it() trees, while the
 * expected-behavior constructors register a single test case in the current
 * target suite.
 */
class TestRegistration
{
  public:
	// Ctor
	explicit
	TestRegistration (std::invocable auto&& callback)
		{ callback(); }

	// Ctor
	TestRegistration (std::string_view expected_behavior, TestSuite::TestCallback1);

	// Ctor
	TestRegistration (std::string_view expected_behavior, TestSuite::TestCallback2);
};


/**
 * Return the default global test suite.
 */
[[nodiscard]]
TestSuite&
default_test_suite();


/**
 * Return the suite currently targeted by free describe()/it() registration.
 *
 * Outside nested registration callbacks this is the default global test suite.
 * While a describe() callback is being registered, it points at the innermost
 * active suite so free describe()/it() calls attach to that subtree.
 */
[[nodiscard]]
TestSuite&
active_test_suite();


/**
 * Add a nested test suite to the current registration target.
 */
void
describe (std::string_view tested_thing, TestSuite::DescribeCallback1);


/**
 * Add a nested test suite to the current registration target.
 */
void
describe (std::string_view tested_thing, TestSuite::DescribeCallback2);

} // namespace neutrino

#include "test_suite_detail.h"

namespace neutrino {

template<class ...Args>
	requires (sizeof...(Args) >= 3)
	inline void
	TestSuite::describe (Args&& ...args)
	{
		auto arguments = std::make_tuple (std::forward<Args> (args)...);
		auto path = test_suite_detail::make_describe_path (arguments, std::make_index_sequence<sizeof...(Args) - 1> {});
		auto callback = std::move (std::get<sizeof...(Args) - 1> (arguments));

		make_active_for ([path = std::move (path), callback = std::move (callback)]() mutable {
			test_suite_detail::describe_with_path (std::move (path), 0, std::move (callback));
		});
	}


/**
 * Add a nested path of test suites to the current registration target.
 */
template<class ...Args>
	requires test_suite_detail::MultiDescribeArguments<Args...>
	inline void
	describe (Args&& ...args)
	{
		active_test_suite().describe (std::forward<Args> (args)...);
	}


/**
 * Add a test case to the current registration target.
 */
void
it (std::string_view expected_behavior, TestSuite::TestCallback1);


/**
 * Add a test case to the current registration target.
 */
void
it (std::string_view expected_behavior, TestSuite::TestCallback2);

} // namespace neutrino

#endif

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

// Local:
#include "test_suite.h"

// Neutrino:
#include <neutrino/exception_support.h>
#include <neutrino/format.h>
#include <neutrino/string.h>
#include <neutrino/time.h>
#include <neutrino/variant.h>

// Standard:
#include <cstddef>
#include <iostream>
#include <sstream>


namespace neutrino {

static constexpr std::string_view	kResetColor			= "\033[0m";
static constexpr std::string_view	kSuccessColor		= "\033[38;2;100;255;100m";
static constexpr std::string_view	kFailureColor		= "\033[38;2;255;90;90m";
static constexpr std::string_view	kDiagnosticColor	= "\033[38;2;225;210;150m";

static constexpr std::string_view	kDescribeSymbol		= "▶";
static constexpr std::string_view	kSuccessSymbol		= "✔";
static constexpr std::string_view	kFailureSymbol		= "❌";
static constexpr std::string_view	kIndentStr			= "    ";

namespace global {

static thread_local TestSuite* active_test_suite = &default_test_suite();

} // namespace global


/**
 * Temporarily make a suite the target for free describe()/it() calls.
 *
 * Scopes can be nested: each instance restores the previously active suite on
 * destruction, so nested describe() callbacks register their children in the
 * right parent.
 */
class PushActiveTestSuite
{
  public:
	// Ctor
	explicit
	PushActiveTestSuite (TestSuite& test_suite):
		_previous_active_test_suite (global::active_test_suite)
	{
		global::active_test_suite = &test_suite;
	}

	// Dtor
	~PushActiveTestSuite()
	{
		global::active_test_suite = _previous_active_test_suite;
	}

  private:
	TestSuite* _previous_active_test_suite;
};


/**
 * Print multi-line diagnostic text with the requested indentation and diagnostic
 * color applied to every line.
 */
static void
print_indented (std::ostream& output, std::string_view const indent, std::string_view const message)
{
	std::istringstream lines { std::string (message) };
	std::string line;

	while (std::getline (lines, line))
		output << indent << kDiagnosticColor << line << kResetColor << '\n';
}


TestSuite::TestSuite (std::string_view name):
	TestSuite (name, nullptr, false)
{ }


TestSuite::TestSuite (std::string_view const name, TestSuite* parent, bool const hidden):
	_parent (parent),
	_hidden (hidden),
	_name (name)
{ }


void
TestSuite::describe (std::string_view const tested_thing, DescribeCallback1 callback)
{
	TestSuite& child = get_or_add_child_suite (tested_thing);

	try {
		PushActiveTestSuite const active_scope (child);
		callback();
	}
	catch (...)
	{
		child._registration_exceptions.push_back (std::current_exception());
	}
}


void
TestSuite::describe (std::string_view const tested_thing, DescribeCallback2 callback)
{
	TestSuite& child = get_or_add_child_suite (tested_thing);

	try {
		PushActiveTestSuite const active_scope (child);
		callback (child);
	}
	catch (...)
	{
		child._registration_exceptions.push_back (std::current_exception());
	}
}


void
TestSuite::make_active_for (DescribeCallback1 callback)
{
	PushActiveTestSuite const active_scope (*this);
	callback();
}


void
TestSuite::it (std::string_view const expected_behavior, TestCallback1 callback)
{
	_steps.emplace_back (ItStep {
		.expected_behavior = std::string (expected_behavior),
		.callback = std::move (callback),
	});
}


void
TestSuite::it (std::string_view const expected_behavior, TestCallback2 callback)
{
	_steps.emplace_back (ItStep {
		.expected_behavior = std::string (expected_behavior),
		.callback = std::move (callback),
	});
}


TestSuite&
TestSuite::get_or_add_child_suite (std::string_view const name)
{
	for (auto& step: _steps)
	{
		if (auto* describe_step = std::get_if<DescribeStep> (&step);
			describe_step && describe_step->test_suite->_name == name)
		{
			return *describe_step->test_suite;
		}
	}

	_steps.emplace_back (DescribeStep { .test_suite = std::unique_ptr<TestSuite> (new TestSuite (name, this, false)) });
	return *std::get<DescribeStep> (_steps.back()).test_suite;
}


bool
TestSuite::execute() const
{
	return execute (std::cout);
}


bool
TestSuite::execute (std::ostream& output) const
{
	return execute (output, compute_indent (_parent));
}


bool
TestSuite::execute (std::ostream& output, uint32_t const indent) const
{
	auto const own_indent = repeat (kIndentStr, indent);
	auto const child_indent = _hidden ? indent : indent + 1;
	auto const t0 = steady_now();
	bool success = true;

	if (!_hidden)
		output << own_indent << kDescribeSymbol << " " << _name << '\n';

	if (!_registration_exceptions.empty())
	{
		success = false;

		for (auto const& exception: _registration_exceptions)
			print_indented (output, repeat (kIndentStr, child_indent), describe_exception (exception));
	}

	for (auto const& step: _steps)
	{
		success &= std::visit (overload {
			[&] (DescribeStep const& describe_step) {
				return describe_step.test_suite->execute (output, child_indent);
			},
			[&] (ItStep const& it_step) {
				return execute_it_step (it_step, output, child_indent);
			},
		}, step);
	}

	if (!_hidden)
	{
		auto const duration = steady_now() - t0;
		auto const color = success ? kSuccessColor : kFailureColor;
		output << own_indent << color << (success ? kSuccessSymbol : kFailureSymbol) << " "
			   << _name << kResetColor << " (" << format_unit (duration, 3) << ")\n";
	}

	return success;
}


bool
TestSuite::execute_it_step (ItStep const& it_step, std::ostream& output, uint32_t const indent)
{
	auto const own_indent = repeat (kIndentStr, indent);
	auto const t0 = steady_now();
	Test test;
	std::exception_ptr exception;

	try {
		std::visit (overload {
			[&] (TestCallback1 const& callback) {
				callback();
			},
			[&] (TestCallback2 const& callback) {
				callback (test);
			},
		}, it_step.callback);
	}
	catch (...)
	{
		exception = std::current_exception();
	}

	auto const success = test.passing() && !exception;
	auto const duration = steady_now() - t0;
	auto const color = success ? kSuccessColor : kFailureColor;
	output << own_indent << color << (success ? kSuccessSymbol : kFailureSymbol) << " "
		   << it_step.expected_behavior << kResetColor << " (" << format_unit (duration, 3) << ")\n";

	if (auto const test_output = test.cout.str();
		!test_output.empty())
	{
		print_indented (output, own_indent + "  ", test_output);
	}

	if (exception)
		print_indented (output, own_indent + "  ", describe_exception (exception));

	return success;
}


uint32_t
TestSuite::compute_indent (TestSuite* parent_test_suite)
{
	uint32_t count = 0;

	while (parent_test_suite != nullptr)
	{
		++count;
		parent_test_suite = parent_test_suite->parent();
	}

	return count;
}


TestRegistration::TestRegistration (std::string_view const expected_behavior, TestSuite::TestCallback1 callback)
{
	active_test_suite().it (expected_behavior, std::move (callback));
}


TestRegistration::TestRegistration (std::string_view const expected_behavior, TestSuite::TestCallback2 callback)
{
	active_test_suite().it (expected_behavior, std::move (callback));
}


void
describe (std::string_view const tested_thing, TestSuite::DescribeCallback1 callback)
{
	active_test_suite().describe (tested_thing, std::move (callback));
}


void
describe (std::string_view const tested_thing, TestSuite::DescribeCallback2 callback)
{
	active_test_suite().describe (tested_thing, std::move (callback));
}


void
it (std::string_view const expected_behavior, TestSuite::TestCallback1 callback)
{
	active_test_suite().it (expected_behavior, std::move (callback));
}


void
it (std::string_view const expected_behavior, TestSuite::TestCallback2 callback)
{
	active_test_suite().it (expected_behavior, std::move (callback));
}


TestSuite&
default_test_suite()
{
	static TestSuite suite ("default test suite", nullptr, true);
	return suite;
}


TestSuite&
active_test_suite()
{
	return *global::active_test_suite;
}

} // namespace neutrino

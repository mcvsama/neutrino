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

static constexpr std::string_view	kSummarySuccessSymbol	= "🟩";
static constexpr std::string_view	kSummaryFailureSymbol	= "❌";

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


TestSuite::ExecutionSummary
TestSuite::execute (std::ostream& output) const
{
	ExecutionSummary summary;
	FailureReport failure_report;
	execute (output, compute_indent (_parent), summary, &failure_report);
	auto const symbol = summary.failing == 0 ? kSummarySuccessSymbol : kSummaryFailureSymbol;
	auto const color = summary.failing == 0 ? kSuccessColor : kFailureColor;
	auto const summary_suffix = failure_report.empty() ? "" : ":";
	output << "\n";
	output << symbol << " " << color << summary.passing << " passing, " << summary.failing << " failing" << summary_suffix << kResetColor << '\n';

	if (!failure_report.empty())
	{
		output << "\n";
		print_failure_report (output, failure_report, compute_indent (_parent));
	}

	return summary;
}


bool
TestSuite::execute (std::ostream& output, uint32_t const indent, ExecutionSummary& summary, FailureReport* const failure_report) const
{
	auto const own_indent = repeat (kIndentStr, indent);
	auto const child_indent = _hidden ? indent : indent + 1;
	auto const t0 = steady_now();
	bool success = true;

	if (failure_report != nullptr)
	{
		failure_report->name = _name;
		failure_report->hidden = _hidden;
		failure_report->duration = si::Time();
		failure_report->steps.clear();
	}

	if (!_hidden)
		output << own_indent << kDescribeSymbol << " " << _name << '\n';

	if (!_registration_exceptions.empty())
	{
		success = false;
		summary.failing += _registration_exceptions.size();

		for (auto const& exception: _registration_exceptions)
			print_indented (output, repeat (kIndentStr, child_indent), describe_exception (exception));
	}

	for (auto const& step: _steps)
	{
		success &= std::visit (overload {
			[&] (DescribeStep const& describe_step) {
				FailureReport child_failure_report;
				auto const child_success = describe_step.test_suite->execute (output, child_indent, summary, &child_failure_report);

				if (failure_report != nullptr && !child_failure_report.empty())
				{
					failure_report->steps.emplace_back (FailureDescribeStep {
						.test_suite = std::make_unique<FailureReport> (std::move (child_failure_report)),
					});
				}

				return child_success;
			},
			[&] (ItStep const& it_step) {
				FailedItStep failed_it_step;
				auto const it_success = execute_it_step (it_step, output, child_indent, summary, &failed_it_step);

				if (failure_report != nullptr && !it_success)
					failure_report->steps.emplace_back (std::move (failed_it_step));

				return it_success;
			},
		}, step);
	}

	if (!_hidden)
	{
		auto const duration = steady_now() - t0;
		auto const color = success ? kSuccessColor : kFailureColor;
		output << own_indent << color << (success ? kSuccessSymbol : kFailureSymbol) << " "
			   << _name << kResetColor << " (" << format_unit (duration, 3) << ")\n";

		if (failure_report != nullptr && !failure_report->empty())
			failure_report->duration = duration;
	}

	return success;
}


bool
TestSuite::execute_it_step (ItStep const& it_step,
							std::ostream& output,
							uint32_t const indent,
							ExecutionSummary& summary,
							FailedItStep* const failed_it_step)
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

	if (success)
		++summary.passing;
	else
		++summary.failing;

	auto const duration = steady_now() - t0;
	auto const color = success ? kSuccessColor : kFailureColor;
	output << own_indent << color << (success ? kSuccessSymbol : kFailureSymbol) << " "
		   << it_step.expected_behavior << kResetColor << " (" << format_unit (duration, 3) << ")\n";

	if (auto const test_output = test.cout.str();
		!test_output.empty())
	{
		print_indented (output, own_indent + "  ", test_output);

		if (!success && failed_it_step != nullptr)
			failed_it_step->diagnostics.push_back (test_output);
	}

	if (exception)
	{
		print_indented (output, own_indent + "  ", describe_exception (exception));

		if (!success && failed_it_step != nullptr)
			failed_it_step->diagnostics.push_back (describe_exception (exception));
	}

	if (!success && failed_it_step != nullptr)
	{
		failed_it_step->expected_behavior = it_step.expected_behavior;
		failed_it_step->duration = duration;
	}

	return success;
}


void
TestSuite::print_failure_report (std::ostream& output, FailureReport const& failure_report, uint32_t const indent)
{
	auto const own_indent = repeat (kIndentStr, indent);
	auto const child_indent = failure_report.hidden ? indent : indent + 1;

	if (!failure_report.hidden)
		output << own_indent << kDescribeSymbol << " " << failure_report.name << '\n';

	for (auto const& step: failure_report.steps)
	{
		std::visit (overload {
			[&] (FailureDescribeStep const& describe_step) {
				print_failure_report (output, *describe_step.test_suite, child_indent);
			},
			[&] (FailedItStep const& failed_it_step) {
				output << repeat (kIndentStr, child_indent) << kFailureColor << kFailureSymbol << " "
					   << failed_it_step.expected_behavior << kResetColor << " ("
					   << format_unit (failed_it_step.duration, 3) << ")\n";

				for (auto const& diagnostic: failed_it_step.diagnostics)
					print_indented (output, repeat (kIndentStr, child_indent) + "  ", diagnostic);
			},
		}, step);
	}

	if (!failure_report.hidden)
		output << own_indent << kFailureColor << kFailureSymbol << " "
			   << failure_report.name << kResetColor << " (" << format_unit (failure_report.duration, 3) << ")\n";
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

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

#ifndef NEUTRINO__TEST__MANUAL_TEST_H__INCLUDED
#define NEUTRINO__TEST__MANUAL_TEST_H__INCLUDED

// Local:
#include "test_asserts.h"
#include "stdexcept.h"

// Neutrino:
#include <neutrino/exception.h>
#include <neutrino/logger.h>

// Standard:
#include <cstddef>
#include <functional>
#include <iostream>
#include <sstream>


namespace neutrino {

class ManualTest
{
	using TestFunction	= std::function<void()>;

	struct Test
	{
		std::string		name;
		TestFunction	function;
	};

  public:
	ManualTest (std::string const& test_name, TestFunction);

  public:
	static bool
	execute (Test const&);

	static void
	run_all();

	static std::vector<Test>&
	tests();
};


inline
ManualTest::ManualTest (std::string const& test_name, TestFunction tf)
{
	tests().push_back ({ test_name, tf });
}


inline bool
ManualTest::execute (Test const& test)
{
	static constexpr char kResetColor[]			= "\033[31;1;0m";
	static constexpr char kFinishedColor[]		= "\033[38;2;220;220;220m";
	static constexpr char kFailColor[]			= "\033[38;2;255;0;0m";
	static constexpr char kExplanationColor[]	= "\033[38;2;225;210;150m";

	std::cout << "Manual test: " << test.name << "…" << std::flush;
	std::ostringstream log_buffer;

	LoggerOutput logger_output (log_buffer);
	logger_output.set_timestamps_enabled (false);
	Logger logger (logger_output);

	bool was_exception = Exception::catch_and_log (logger, [&] {
		test.function();
		std::cout << " " << kFinishedColor << "FINISHED" << kResetColor << std::endl;
	});

	if (was_exception)
	{
		std::cout << " " << kFailColor << "FAIL" << kResetColor << std::endl;
		std::cout << kExplanationColor << "Explanation: " << log_buffer.str() << kResetColor << std::endl;
	}

	return !was_exception;
}


inline void
ManualTest::run_all()
{
	for (auto const& test: ManualTest::tests())
		execute (test);
}


inline auto
ManualTest::tests()
	-> std::vector<Test>&
{
	static std::vector<Test> tests;
	return tests;
}

} // namespace neutrino

#endif


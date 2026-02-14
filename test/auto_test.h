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

#ifndef NEUTRINO__TEST__AUTO_TEST_H__INCLUDED
#define NEUTRINO__TEST__AUTO_TEST_H__INCLUDED

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

class AutoTest
{
	using TestFunction	= std::function<void()>;

	struct Test
	{
		std::string		name;
		TestFunction	function;
	};

  public:
	AutoTest (std::string const& test_name, TestFunction);

  public:
	static bool
	execute (Test const&);

	static bool
	run_all();

	static std::vector<Test>&
	tests();
};


inline
AutoTest::AutoTest (std::string const& test_name, TestFunction tf)
{
	tests().push_back ({ test_name, tf });
}


inline bool
AutoTest::execute (Test const& test)
{
	static constexpr char kResetColor[]			= "\033[31;1;0m";
	static constexpr char kPassColor[]			= "\033[38;2;100;255;100m";
	static constexpr char kFailColor[]			= "\033[38;2;255;0;0m";
	static constexpr char kExplanationColor[]	= "\033[38;2;225;210;150m";

	std::cout << "Auto test: " << test.name << "…" << std::flush;
	std::ostringstream log_buffer;

	LoggerOutput logger_output (log_buffer);
	logger_output.set_timestamps_enabled (false);
	Logger logger (logger_output);

	bool was_exception = Exception::catch_and_log (logger, [&]{
		test.function();
		std::cout << " " << kPassColor << "PASS" << kResetColor << std::endl;
	});

	if (was_exception)
	{
		std::cout << " " << kFailColor << "FAIL" << kResetColor << std::endl;
		std::cout << kExplanationColor << "Explanation: " << log_buffer.str() << kResetColor << std::endl;
	}

	return !was_exception;
}


inline bool
AutoTest::run_all()
{
	bool success = true;

	for (auto const& test: AutoTest::tests())
		success &= execute (test);

	return success;
}


inline auto
AutoTest::tests()
	-> std::vector<Test>&
{
	static std::vector<Test> tests;
	return tests;
}

} // namespace neutrino

#endif

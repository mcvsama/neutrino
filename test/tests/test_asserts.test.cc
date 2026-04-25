/* vim:ts=4
 *
 * Copyleft 2026  Michal Gawron
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
#include <neutrino/test/test_asserts.h>
#include <neutrino/test/test_suite.h>

// Standard:
#include <stdexcept>
#include <string>


namespace neutrino::test {
namespace {

TestRegistration tests ([] {
	describe ("Neutrino", "test_asserts", [] {
		it ("verify_throws returns a rethrowable exception_ptr", []{
			auto const exception = test_asserts::verify_throws<std::runtime_error> (
				"verify_throws should capture the expected exception",
				[] {
					throw std::runtime_error ("saved exception");
				});

			test_asserts::verify ("returned exception_ptr is not empty", static_cast<bool> (exception));

			bool caught_expected_exception = false;

			try {
				std::rethrow_exception (exception);
			}
			catch (std::runtime_error const& e)
			{
				caught_expected_exception = true;
				test_asserts::verify_equal ("rethrow preserves exception message", std::string (e.what()), "saved exception");
			}
			catch (...)
			{
				test_asserts::verify ("rethrow keeps the original exception type", false);
			}

			test_asserts::verify ("rethrow preserves the original exception type", caught_expected_exception);
		});
	});
});

} // namespace
} // namespace neutrino::test

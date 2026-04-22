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
#include <neutrino/string.h>
#include <neutrino/test/test_asserts.h>
#include <neutrino/test/test_suite.h>

namespace neutrino::test {
namespace {

TestRegistration tests ([]{
	describe ("Neutrino", "string", []{
		it ("repeat()", []{
			test_asserts::verify_equal ("empty string repeated zero times", repeat ("", 0), "");
			test_asserts::verify_equal ("empty string repeated several times", repeat ("", 5), "");
			test_asserts::verify_equal ("string repeated zero times", repeat ("abc", 0), "");
			test_asserts::verify_equal ("single-character string repeated several times", repeat (" ", 5), "     ");
			test_asserts::verify_equal ("multi-character string repeated several times", repeat ("ab", 3), "ababab");
		});
	});
});

} // namespace
} // namespace neutrino::test

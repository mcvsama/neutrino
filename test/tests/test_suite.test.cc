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
#include <cstddef>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace neutrino::test {
namespace {

static constexpr std::string_view kResetColor = "\033[0m";
static constexpr std::string_view kSuccessColor = "\033[38;2;100;255;100m";
static constexpr std::string_view kFailureColor = "\033[38;2;255;90;90m";


static std::size_t
count_occurrences (std::string const& haystack, std::string_view const needle)
{
	std::size_t count = 0;
	std::size_t pos = 0;

	while ((pos = haystack.find (needle, pos)) != std::string::npos)
	{
		++count;
		pos += needle.size();
	}

	return count;
}


TestRegistration tests ([] {
	describe ("Neutrino", "TestSuite", [] {
		it ("prints nested describe()/it() output", []{
			TestSuite suite ("Root");

			suite.describe ("Group", [] (TestSuite& group) {
				group.it ("passes", [] {
				});

				group.it ("fails", [] (Test& test) {
					test.cout << "diagnostic output";
					test.set_passing (false);
				});
			});

			suite.describe ("No-arg describe", [] {
				it ("uses active registration context", [] {
				});
			});

			std::ostringstream output;
			auto const summary = suite.execute (output);
			auto const report = output.str();

			test_asserts::verify ("suite reports failure when a child test fails", !summary.success());
			test_asserts::verify_equal ("suite counts passing tests", summary.passing, 2u);
			test_asserts::verify_equal ("suite counts failing tests", summary.failing, 1u);
			test_asserts::verify ("suite prints root describe start", report.find ("▶ Root") != std::string::npos);
			test_asserts::verify ("suite prints nested describe start", report.find ("▶ Group") != std::string::npos);
			test_asserts::verify ("suite prints passing test", report.find ("✔ passes") != std::string::npos);
			test_asserts::verify ("suite prints failing test", report.find ("❌ fails") != std::string::npos);
			test_asserts::verify ("suite prints test diagnostics", report.find ("diagnostic output") != std::string::npos);
			test_asserts::verify ("suite prints failing nested describe summary", report.find ("❌ Group") != std::string::npos);
			test_asserts::verify ("suite prints no-arg describe child test", report.find ("✔ uses active registration context") != std::string::npos);
			test_asserts::verify ("suite prints failing root summary", report.find ("❌ Root") != std::string::npos);
			test_asserts::verify ("suite prints red passing/failing summary",
								  report.find (std::string (kFailureColor) + "2 passing, 1 failing:" + std::string (kResetColor)) != std::string::npos);
			test_asserts::verify ("suite does not print separate failing tests replay header", report.find ("Failing tests:") == std::string::npos);
			test_asserts::verify_equal ("passing tests are not replayed", count_occurrences (report, "✔ passes"), 1u);
			test_asserts::verify_equal ("failing test is replayed once after the main run", count_occurrences (report, "❌ fails"), 2u);
			test_asserts::verify_equal ("failing test diagnostics are replayed once after the main run", count_occurrences (report, "diagnostic output"), 2u);
			test_asserts::verify_equal ("failing describe path is replayed once after the main run", count_occurrences (report, "\n    ▶ Group"), 2u);
		});

		it ("merges describe() only at the same path", []{
			TestSuite suite ("Root");

			suite.describe ("A", [] {
				describe ("B", [] {
					describe ("C", [] {
						it ("first A/B/C test", [] {
						});
					});
				});
			});

			suite.describe ("B", [] {
				describe ("C", [] {
					it ("B/C test", [] {
					});
				});
			});

			suite.describe ("A", [] {
				describe ("B", [] {
					describe ("C", [] {
						it ("second A/B/C test", [] {
						});
					});
				});
			});

			std::ostringstream output;
			auto const summary = suite.execute (output);
			auto const report = output.str();

			test_asserts::verify ("suite with merged describe paths passes", summary.success());
			test_asserts::verify_equal ("merged describe suite counts passing tests", summary.passing, 3u);
			test_asserts::verify_equal ("merged describe suite counts failing tests", summary.failing, 0u);
			test_asserts::verify_equal ("top-level A is printed once", count_occurrences (report, "\n    ▶ A"), 1u);
			test_asserts::verify_equal ("top-level B is printed once", count_occurrences (report, "\n    ▶ B"), 1u);
			test_asserts::verify_equal ("A/B/C path is printed once", count_occurrences (report, "\n            ▶ C"), 1u);
			test_asserts::verify_equal ("B/C path is printed once", count_occurrences (report, "\n        ▶ C"), 1u);
			test_asserts::verify ("first A/B/C test is present", report.find ("✔ first A/B/C test") != std::string::npos);
			test_asserts::verify ("second A/B/C test is present", report.find ("✔ second A/B/C test") != std::string::npos);
			test_asserts::verify ("B/C test is present", report.find ("✔ B/C test") != std::string::npos);
		});

		it ("supports multi-component describe() paths", [] {
			TestSuite suite ("Root");

			suite.describe ("A", [] {
				describe ("B", "C", [] {
					it ("multi-component path test", [] {
					});
				});
			});

			suite.describe ("X", "Y", [] {
				it ("member multi-component path test", [] {
				});
			});

			std::ostringstream output;
			auto const summary = suite.execute (output);
			auto const report = output.str();

			test_asserts::verify ("suite with multi-component describe path passes", summary.success());
			test_asserts::verify_equal ("multi-component suite counts passing tests", summary.passing, 2u);
			test_asserts::verify_equal ("multi-component suite counts failing tests", summary.failing, 0u);
			test_asserts::verify ("first nested suite is printed", report.find ("\n    ▶ A") != std::string::npos);
			test_asserts::verify ("second nested suite is printed", report.find ("▶ B") != std::string::npos);
			test_asserts::verify ("third nested suite is printed", report.find ("▶ C") != std::string::npos);
			test_asserts::verify ("test under multi-component path is printed", report.find ("✔ multi-component path test") != std::string::npos);
			test_asserts::verify ("member multi-component suite is printed", report.find ("▶ X") != std::string::npos);
			test_asserts::verify ("member nested suite is printed", report.find ("▶ Y") != std::string::npos);
			test_asserts::verify ("member multi-component path test is printed", report.find ("✔ member multi-component path test") != std::string::npos);
			test_asserts::verify ("suite prints green passing/failing summary",
								  report.find (std::string (kSuccessColor) + "2 passing, 0 failing" + std::string (kResetColor)) != std::string::npos);
		});

		it ("prints all registration exceptions from merged describe() calls", []{
			TestSuite suite ("Root");

			suite.describe ("Broken", [] {
				throw std::runtime_error ("first registration failure");
			});

			suite.describe ("Broken", [] {
				throw std::runtime_error ("second registration failure");
			});

			std::ostringstream output;
			auto const summary = suite.execute (output);
			auto const report = output.str();

			test_asserts::verify ("suite reports failure when registration callbacks fail", !summary.success());
			test_asserts::verify_equal ("registration failures are counted", summary.failing, 2u);
			test_asserts::verify_equal ("merged failing describe is printed once", count_occurrences (report, "\n    ▶ Broken"), 1u);
			test_asserts::verify ("first registration exception is printed", report.find ("first registration failure") != std::string::npos);
			test_asserts::verify ("second registration exception is printed", report.find ("second registration failure") != std::string::npos);
			test_asserts::verify ("merged failing describe summary is printed", report.find ("❌ Broken") != std::string::npos);
			test_asserts::verify ("registration exceptions contribute to failing summary",
								  report.find (std::string (kFailureColor) + "0 passing, 2 failing" + std::string (kResetColor)) != std::string::npos);
		});
	});
});

} // namespace
} // namespace neutrino::test

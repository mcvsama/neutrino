/* vim:ts=4
 *
 * Copyleft 2008…2013  Michał Gawron
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
#include <neutrino/numeric.h>
#include <neutrino/si/si.h>

// Standard:
#include <algorithm>
#include <array>
#include <cstddef>
#include <stdexcept>
#include <utility>
#include <vector>


namespace neutrino::test {
namespace {

TestRegistration tests ([]{
	describe ("Neutrino", "numeric", []{
		it ("median functions handle an unordered lower partition", []{
			std::array<double, 6> samples { 0.0, 1.0, 2.0, 4.0, 3.0, 5.0 };
			auto const original = samples;

			test_asserts::verify_equal ("median averages the two middle order statistics", median (samples.begin(), samples.end()), 2.5);
			test_asserts::verify_equal ("median preserves the input order", samples, original);
			test_asserts::verify_equal ("sort_and_median averages the two middle order statistics", sort_and_median (samples.begin(), samples.end()), 2.5);
		});

		it ("median functions are independent of input order", []{
			std::array<double, 6> samples { 0.0, 1.0, 2.0, 3.0, 4.0, 5.0 };

			do {
				auto const original = samples;
				auto mutable_samples = samples;

				test_asserts::verify_equal ("median is correct for every permutation", median (samples.begin(), samples.end()), 2.5);
				test_asserts::verify_equal ("median preserves each permutation", samples, original);
				test_asserts::verify_equal ("sort_and_median is correct for every permutation", sort_and_median (mutable_samples.begin(), mutable_samples.end()), 2.5);
			} while (std::next_permutation (samples.begin(), samples.end()));
		});

		it ("median functions handle singletons, pairs, odd counts, and repeated values", []{
			std::array<std::pair<std::vector<double>, double>, 6> const cases {{
				{ { 42.0 }, 42.0 },
				{ { 4.0, 2.0 }, 3.0 },
				{ { 9.0, 1.0, 5.0 }, 5.0 },
				{ { -5.0, -3.0, -1.0, -4.0 }, -3.5 },
				{ { 2.0, 1.0, 2.0, 3.0 }, 2.0 },
				{ { 2.0, 2.0, 1.0, 3.0, 2.0 }, 2.0 },
			}};

			for (auto const& [samples, expected]: cases)
			{
				auto mutable_samples = samples;

				test_asserts::verify_equal ("median is correct for this sample count", median (samples.begin(), samples.end()), expected);
				test_asserts::verify_equal ("sort_and_median is correct for this sample count", sort_and_median (mutable_samples.begin(), mutable_samples.end()), expected);
			}
		});

		it ("median functions reject empty ranges", []{
			std::array<double, 0> samples;

			test_asserts::verify_throws<std::length_error> ("median rejects an empty range", [&samples] {
				static_cast<void> (median (samples.begin(), samples.end()));
			});
			test_asserts::verify_throws<std::length_error> ("sort_and_median rejects an empty range", [&samples] {
				static_cast<void> (sort_and_median (samples.begin(), samples.end()));
			});
		});

		it ("median functions support physical quantities", []{
			using namespace si::literals;

			std::array<si::Time, 6> even_samples { 0_s, 1_s, 2_s, 4_s, 3_s, 5_s };
			std::array<si::Time, 3> odd_samples { 3_s, 1_s, 2_s };

			test_asserts::verify_equal ("even median keeps its physical unit", median (even_samples.begin(), even_samples.end()), 2.5_s);
			test_asserts::verify_equal ("even sort_and_median keeps its physical unit", sort_and_median (even_samples.begin(), even_samples.end()), 2.5_s);
			test_asserts::verify_equal ("odd median keeps its physical unit", median (odd_samples.begin(), odd_samples.end()), 2_s);
			test_asserts::verify_equal ("odd sort_and_median keeps its physical unit", sort_and_median (odd_samples.begin(), odd_samples.end()), 2_s);
		});

		it ("trapezoid_integral()", []{
			auto int1 = trapezoid_integral (static_cast<double (*)(double)> (std::sin), { -5.3, +12.0 }, 1e-5);

			test_asserts::verify_equal_with_epsilon ("integrated sin() is correct", int1, -0.28948, 1e-5);
		});

		it ("renormalize()", []{
			using namespace si::literals;

			// Should compile:
			[[maybe_unused]] float x = renormalize (1_rad, Range { 0_deg, 90_deg }, Range { 0.0f, 1.0f });
			[[maybe_unused]] float y = renormalize (1_rad, { 0_deg, 90_deg }, { 0.0f, 1.0f });
		});
	});
});

} // namespace
} // namespace neutrino::test

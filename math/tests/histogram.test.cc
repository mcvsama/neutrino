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
#include <neutrino/math/histogram.h>
#include <neutrino/test/test_asserts.h>
#include <neutrino/test/test_suite.h>

// Standard:
#include <array>
#include <cmath>
#include <cstddef>
#include <numeric>
#include <optional>


namespace neutrino::test {
namespace {

TestRegistration tests ([]{
	describe ("Neutrino", "math", "Histogram", []{
		it ("includes the upper boundary in the last bin", []{
			std::array<double, 3> const samples { 0.0, 1.0, 2.0 };
			math::Histogram<double> const histogram (samples.begin(), samples.end(), {
				.bin_width = 1.0,
				.min_x = std::nullopt,
				.max_x = std::nullopt,
			});

			test_asserts::verify_equal ("all boundary samples are counted", histogram.bins(), math::Histogram<double>::Bins { 1, 2 });
			test_asserts::verify_equal ("upper boundary maps to the last bin", histogram.bin_index (2.0), std::size_t (1));
			test_asserts::verify_equal ("largest bin includes the maximum sample", histogram.max_y(), std::size_t (2));
			test_asserts::verify_equal ("bin counts match the number of samples",
										std::accumulate (histogram.bins().begin(),
														 histogram.bins().end(),
														 std::size_t (0)),
										histogram.n_samples());
			test_asserts::verify_equal ("fraction at the minimum includes every sample", histogram.normalized_percentile_for (histogram.min_x()), 1.0f);
		});

		it ("includes an explicit upper boundary while excluding values above it", []{
			std::array<double, 4> const samples { 0.0, 1.0, 2.0, 3.0 };
			math::Histogram<double> const histogram (samples.begin(), samples.end(), {
				.bin_width = 1.0,
				.min_x = 0.0,
				.max_x = 2.0,
			});

			test_asserts::verify_equal ("only samples within the specified range are counted", histogram.bins(), math::Histogram<double>::Bins { 1, 2 });
			test_asserts::verify ("values immediately above the upper boundary remain outside the bins",
								  histogram.bin_index (std::nextafter (2.0, 3.0)) >= histogram.bins().size());
			test_asserts::verify_equal ("fraction uses all input samples as the denominator", histogram.normalized_percentile_for (0.0), 0.75f);
		});

		it ("keeps the bin count when the range is not a multiple of bin width", []{
			std::array<double, 4> const samples { 0.0, 1.0, 2.0, 2.5 };
			math::Histogram<double> const histogram (samples.begin(), samples.end(), {
				.bin_width = 1.0,
				.min_x = std::nullopt,
				.max_x = std::nullopt,
			});

			test_asserts::verify_equal ("partially filled final bin includes the upper boundary", histogram.bins(), math::Histogram<double>::Bins { 1, 1, 2 });
			test_asserts::verify_equal ("fraction at the minimum includes every sample", histogram.normalized_percentile_for (0.0), 1.0f);
		});

		it ("includes the maximum for a range starting below zero", []{
			std::array<double, 3> const samples { -2.0, -1.0, 0.0 };
			math::Histogram<double> const histogram (samples.begin(), samples.end(), {
				.bin_width = 1.0,
				.min_x = std::nullopt,
				.max_x = std::nullopt,
			});

			test_asserts::verify_equal ("negative range origin preserves bin membership", histogram.bins(), math::Histogram<double>::Bins { 1, 2 });
			test_asserts::verify_equal ("fraction at the minimum includes every sample", histogram.normalized_percentile_for (-2.0), 1.0f);
		});

		it ("includes the upper boundary for physical quantities", []{
			using namespace si::literals;

			std::array<si::Time, 3> const samples { 0_s, 1_s, 2_s };
			math::Histogram<si::Time> const histogram (samples.begin(), samples.end(), {
				.bin_width = 1_s,
				.min_x = std::nullopt,
				.max_x = std::nullopt,
			});

			test_asserts::verify_equal ("physical quantities use the same closed upper boundary", histogram.bins(), math::Histogram<si::Time>::Bins { 1, 2 });
			test_asserts::verify_equal ("upper boundary keeps its physical unit", histogram.max_x(), 2_s);
			test_asserts::verify_equal ("upper boundary maps to the last bin", histogram.bin_index (2_s), std::size_t (1));
			test_asserts::verify_equal ("fraction at the minimum includes every sample", histogram.normalized_percentile_for (0_s), 1.0f);
		});

		it ("uses a single bin when bin width exceeds the range", []{
			std::array<double, 3> const samples { 0.0, 1.0, 2.0 };
			math::Histogram<double> const histogram (samples.begin(), samples.end(), {
				.bin_width = 5.0,
				.min_x = std::nullopt,
				.max_x = std::nullopt,
			});

			test_asserts::verify_equal ("all samples fit in the single bin", histogram.bins(), math::Histogram<double>::Bins { 3 });
			test_asserts::verify_equal ("upper boundary maps to the only bin", histogram.bin_index (2.0), std::size_t (0));
		});
	});
});

} // namespace
} // namespace neutrino::test

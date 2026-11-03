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
#include <limits>
#include <numeric>
#include <optional>
#include <stdexcept>


namespace neutrino::test {
namespace {

TestRegistration tests ([]{
	describe ("Neutrino", "math", "Histogram", []{
		it ("rejects non-finite samples and parameters before computing histogram bounds", []{
			auto const verify_type = []<class Value> {
				using Histogram = math::Histogram<Value>;
				using Number = si::decay_quantity_t<Value>;
				std::array<Value, 3> const samples { Value (0), Value (1), Value (2) };

				for (auto const bad: { std::numeric_limits<Number>::quiet_NaN(), std::numeric_limits<Number>::infinity(), -std::numeric_limits<Number>::infinity() })
				{
					std::array<typename Histogram::Parameters, 3> const invalid_parameters {{
						{ Value (bad), Value (0), Value (2) },
						{ Value (1), Value (bad), Value (2) },
						{ Value (1), Value (0), Value (bad) },
					}};

					for (auto const& parameters: invalid_parameters)
					{
						test_asserts::verify_throws<std::invalid_argument> ("non-finite histogram parameter is rejected", [&] {
							Histogram histogram (samples.begin(), samples.end(), parameters);
						});
					}

					for (std::size_t index = 0; index < samples.size(); ++index)
					{
						auto invalid_samples = samples;
						invalid_samples[index] = Value (bad);
						test_asserts::verify_throws<std::invalid_argument> ("non-finite sample is rejected at every position", [&] {
							Histogram histogram (invalid_samples.begin(), invalid_samples.end(), { Value (1), Value (0), Value (2) });
						});
					}
				}
			};

			verify_type.operator()<double>();
			verify_type.operator()<si::Time>();
		});

		it ("rejects excessive bin counts before conversion or allocation", []{
			auto const verify_type = []<class Value> {
				using Number = si::decay_quantity_t<Value>;
				std::array<Value, 2> const samples { Value (0), Value (1) };
				constexpr auto size_bits = std::numeric_limits<std::size_t>::digits;
				auto const size_limit = std::ldexp (Number (1), size_bits);

				for (auto const width: {
					 Number (1e-300),
					 std::numeric_limits<Number>::denorm_min(),
					 Number (1) / size_limit,
					 Number (1) / std::nextafter (size_limit, Number (0)),
					 std::ldexp (Number (1), 1 - size_bits),
				}) {
					test_asserts::verify_throws<std::length_error> ("unrepresentable or unallocatable bin count is rejected", [&] {
						math::Histogram<Value> histogram (samples.begin(), samples.end(), { Value (width), std::nullopt, std::nullopt });
					});
				}
			};

			verify_type.operator()<double>();
			verify_type.operator()<long double>();
			verify_type.operator()<si::Time>();
		});

		it ("counts and indexes wide finite ranges without overflowing the difference", []{
			auto const verify_type = []<class Value> {
				using Number = si::decay_quantity_t<Value>;
				auto const max = std::numeric_limits<Number>::max();
				std::array<Value, 4> const samples { Value (-max), Value (0), Value (max / 2), Value (max) };
				math::Histogram<Value> const histogram (samples.begin(), samples.end(), { Value (max), std::nullopt, std::nullopt });
				test_asserts::verify_equal ("a range wider than the scalar limit needs only two bins", histogram.bins(), typename math::Histogram<Value>::Bins { 1, 3 });
				test_asserts::verify_equal ("negative interior value belongs to the first bin", histogram.bin_index (Value (-max / 2)), std::size_t (0));
				test_asserts::verify_equal ("zero belongs to the second bin", histogram.bin_index (Value (0)), std::size_t (1));
				test_asserts::verify_equal ("positive interior value is indexed without overflowing", histogram.bin_index (Value (max / 2)), std::size_t (1));
				test_asserts::verify_equal ("upper boundary belongs to the last bin", histogram.bin_index (Value (max)), std::size_t (1));
				test_asserts::verify_equal ("every finite input sample is counted", histogram.normalized_percentile_for (Value (-max)), 1.0f);
			};

			verify_type.operator()<double>();
			verify_type.operator()<si::Time>();
			verify_type.operator()<si::Quantity<si::Kilometer>>();
		});

		it ("keeps one bin when the range-to-width ratio underflows to zero", []{
			auto const verify_type = []<class Value> {
				using Number = si::decay_quantity_t<Value>;
				std::array<Value, 2> const samples { Value (0), Value (std::numeric_limits<Number>::denorm_min()) };
				math::Histogram<Value> const histogram (samples.begin(), samples.end(), { Value (std::numeric_limits<Number>::max()), std::nullopt, std::nullopt });
				test_asserts::verify_equal ("a nonempty histogram always has at least one bin", histogram.bins(), typename math::Histogram<Value>::Bins { 2 });
				test_asserts::verify_equal ("minimum belongs to the only bin", histogram.bin_index (samples.front()), std::size_t (0));
				test_asserts::verify_equal ("maximum belongs to the only bin", histogram.bin_index (samples.back()), std::size_t (0));
			};

			verify_type.operator()<double>();
			verify_type.operator()<si::Time>();
		});

		it ("rejects constant ranges whose upper bound cannot be expanded", []{
			auto const verify_type = []<class Value> {
				using Number = si::decay_quantity_t<Value>;
				auto const max = std::numeric_limits<Number>::max();
				std::array<std::array<Number, 2>, 2> const cases {{ { max, max }, { Number (1e300), Number (1) } }};

				for (auto const& [origin, width]: cases)
				{
					std::array<Value, 2> const samples { Value (origin), Value (origin) };
					test_asserts::verify_throws<std::overflow_error> ("overflow or rounding cannot produce an invalid histogram range", [&] {
						math::Histogram<Value> histogram (samples.begin(), samples.end(), { Value (width), std::nullopt, std::nullopt });
					});
				}
			};

			verify_type.operator()<double>();
			verify_type.operator()<si::Time>();
		});

		it ("preserves finite default widths and expands ordinary constant ranges", []{
			std::array<double, 3> const samples { 0.0, 1.0, 2.0 };

			for (auto const width: { 0.0, -1.0, -std::numeric_limits<double>::max() })
			{
				math::Histogram<double> const histogram (samples.begin(), samples.end(), { width, std::nullopt, std::nullopt });
				test_asserts::verify_equal ("nonpositive finite width still selects one", histogram.bins(), math::Histogram<double>::Bins { 1, 2 });
			}

			std::array<double, 2> const constant_samples { 5.0, 5.0 };
			math::Histogram<double> const constant (constant_samples.begin(), constant_samples.end(), { 2.0, std::nullopt, std::nullopt });
			test_asserts::verify_equal ("constant data still uses one bin", constant.bins(), math::Histogram<double>::Bins { 2 });
			test_asserts::verify_equal ("ordinary constant range is expanded by the bin width", constant.max_x(), 7.0);
		});

		it ("computes the median of an even-length unsorted sequence", []{
			std::array<double, 6> const samples { 0.0, 1.0, 2.0, 4.0, 3.0, 5.0 };
			math::Histogram<double> const histogram (samples.begin(), samples.end(), {
				.bin_width = 1.0,
				.min_x = std::nullopt,
				.max_x = std::nullopt,
			});

			test_asserts::verify_equal ("median averages the two middle sample values", histogram.median(), 2.5);
		});

		it ("computes the median of physical quantities", []{
			using namespace si::literals;

			std::array<si::Time, 6> const samples { 0_s, 1_s, 2_s, 4_s, 3_s, 5_s };
			math::Histogram<si::Time> const histogram (samples.begin(), samples.end(), {
				.bin_width = 1_s,
				.min_x = std::nullopt,
				.max_x = std::nullopt,
			});

			test_asserts::verify_equal ("median keeps its physical unit", histogram.median(), 2.5_s);
		});

		it ("counts all bins for thresholds below the histogram minimum", []{
			std::array<double, 3> const samples { 1.0, 2.0, 3.0 };
			math::Histogram<double> const histogram (samples.begin(), samples.end(), {
				.bin_width = 1.0,
				.min_x = std::nullopt,
				.max_x = std::nullopt,
			});

			for (auto const value: { -2.0, 0.75, std::nextafter (1.0, 0.0), -std::numeric_limits<double>::max(), -std::numeric_limits<double>::infinity() })
			{
				test_asserts::verify_equal ("value below the minimum has no bin", histogram.bin_index (value), histogram.bins().size());
				test_asserts::verify_equal ("threshold below the minimum includes every sample", histogram.normalized_percentile_for (value), 1.0f);
			}
		});

		it ("rejects indices above the range and unordered values before conversion", []{
			std::array<double, 3> const samples { 1.0, 2.0, 3.0 };
			math::Histogram<double> const histogram (samples.begin(), samples.end(), {
				.bin_width = 1.0,
				.min_x = std::nullopt,
				.max_x = std::nullopt,
			});

			for (auto const value: { std::nextafter (3.0, 4.0), std::numeric_limits<double>::max(), std::numeric_limits<double>::infinity(), std::numeric_limits<double>::quiet_NaN() })
			{
				test_asserts::verify_equal ("outside value has no bin", histogram.bin_index (value), histogram.bins().size());
				test_asserts::verify_equal ("outside threshold counts no bins", histogram.normalized_percentile_for (value), 0.0f);
			}
		});

		it ("excludes samples outside explicitly configured histogram bounds", []{
			std::array<double, 8> const samples { -2.0, -0.25, 0.0, 1.0, 2.0, 2.5, 2.75, 4.0 };
			math::Histogram<double> const histogram (samples.begin(), samples.end(), {
				.bin_width = 1.0,
				.min_x = 0.0,
				.max_x = 2.5,
			});

			test_asserts::verify_equal ("only in-range samples are binned", histogram.bins(), math::Histogram<double>::Bins { 1, 1, 2 });
			test_asserts::verify_equal ("input count includes samples outside the displayed range", histogram.n_samples(), samples.size());
			test_asserts::verify_equal ("threshold below the range counts all binned samples", histogram.normalized_percentile_for (-3.0), 0.5f);
			test_asserts::verify_equal ("threshold at the minimum counts the same bins", histogram.normalized_percentile_for (0.0), 0.5f);
		});

		it ("handles values below the minimum with physical quantities", []{
			using namespace si::literals;

			std::array<si::Time, 3> const samples { 1_s, 2_s, 3_s };
			math::Histogram<si::Time> const histogram (samples.begin(), samples.end(), {
				.bin_width = 1_s,
				.min_x = std::nullopt,
				.max_x = std::nullopt,
			});

			for (auto const value: { -2_s, 0_s, 0.75_s })
			{
				test_asserts::verify_equal ("physical quantity below the minimum has no bin", histogram.bin_index (value), histogram.bins().size());
				test_asserts::verify_equal ("physical threshold below the minimum includes every sample", histogram.normalized_percentile_for (value), 1.0f);
			}
		});

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

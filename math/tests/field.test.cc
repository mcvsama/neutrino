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
#include <neutrino/math/math.h>
#include <neutrino/math/field.h>
#include <neutrino/test/test_asserts.h>
#include <neutrino/test/test_suite.h>

// Standard:
#include <array>
#include <cstddef>
#include <optional>
#include <tuple>
#include <type_traits>
#include <utility>


namespace neutrino::test {
namespace {

using namespace neutrino::si::literals;


template<class Field, class Point, class Value>
	void
	verify_extremum (Field const& field, Point const& point, Value const& expected, Value const& epsilon)
	{
		test_asserts::verify ("extremum exists", point.has_value());
		test_asserts::verify_equal_with_epsilon ("extremum has the expected value", point->value, expected, epsilon);

		auto const actual = std::apply ([&] (auto const& ...args) { return field.value (args...); }, point->arguments);

		test_asserts::verify ("extremum is inside the field domain", actual.has_value());
		test_asserts::verify_equal_with_epsilon ("extremum matches the field at its arguments", point->value, *actual, epsilon);
	}


TestRegistration tests ([]{
	describe ("Neutrino", "math", "Field", []{
		it ("computes the codomain from extrema after interpolating a fixed argument", []{
			math::Field<double, double, double> const field {
				{ 0.0, { { 0.0, 0.0 }, { 1.0, 10.0 } } },
				{ 1.0, { { 0.0, 10.0 }, { 1.0, 0.0 } } },
			};
			std::array<std::array<double, 3>, 4> const cases {{
				{ 0.0, 0.0, 10.0 },
				{ 0.25, 2.5, 7.5 },
				{ 0.5, 5.0, 5.0 },
				{ 1.0, 0.0, 10.0 },
			}};

			for (auto const& [x, expected_min, expected_max]: cases)
			{
				auto const codomain = field.codomain (x);
				static_assert (std::is_same_v<decltype (codomain), std::optional<Range<double>> const>);
				test_asserts::verify ("slice has a codomain", codomain.has_value());
				test_asserts::verify_equal ("minimum is computed after interpolation", codomain->min(), expected_min);
				test_asserts::verify_equal ("maximum is computed after interpolation", codomain->max(), expected_max);
			}

			auto const global = field.codomain();
			static_assert (std::is_same_v<decltype (global), Range<double> const>);
			test_asserts::verify_equal ("global minimum is unchanged", global.min(), 0.0);
			test_asserts::verify_equal ("global maximum is unchanged", global.max(), 10.0);
		});

		it ("returns a zero-valued singleton codomain and rejects unavailable fixed arguments", []{
			math::Field<double, double, double> const field {
				{ 0.0, { { 1.0, 0.0 } } },
			};
			auto const codomain = field.codomain (0.0);
			test_asserts::verify ("zero extrema give a defined codomain", codomain.has_value());
			test_asserts::verify_equal ("singleton minimum is zero", codomain->min(), 0.0);
			test_asserts::verify_equal ("singleton maximum is zero", codomain->max(), 0.0);
			test_asserts::verify ("fixed argument below the domain is rejected", !field.codomain (-1.0));
			test_asserts::verify ("fixed argument above the domain is rejected", !field.codomain (1.0));
		});

		it ("returns no codomain for interpolated slices without a common domain", []{
			math::Field<double, double, double> const field {
				{ 0.0, { { 0.0, 1.0 }, { 1.0, 2.0 } } },
				{ 1.0, { { 2.0, 3.0 }, { 3.0, 4.0 } } },
			};
			test_asserts::verify ("disjoint slices have no interpolated codomain", !field.codomain (0.5));

			auto const first = field.codomain (0.0);
			auto const last = field.codomain (1.0);
			test_asserts::verify ("first exact knot has a codomain", first.has_value());
			test_asserts::verify ("last exact knot has a codomain", last.has_value());
			test_asserts::verify_equal ("first slice minimum is correct", first->min(), 1.0);
			test_asserts::verify_equal ("first slice maximum is correct", first->max(), 2.0);
			test_asserts::verify_equal ("last slice minimum is correct", last->min(), 3.0);
			test_asserts::verify_equal ("last slice maximum is correct", last->max(), 4.0);
		});

		it ("preserves physical value units with multiple fixed arguments in codomain", []{
			math::Field<double, si::Angle, si::Time, si::Length> const field {
				{ 0.0, { { 0_deg, { { 0_s, 0_m }, { 1_s, 10_m } } }, { 10_deg, { { 0_s, 20_m }, { 1_s, 30_m } } } } },
				{ 1.0, { { 0_deg, { { 0_s, 10_m }, { 1_s, 0_m } } }, { 10_deg, { { 0_s, 30_m }, { 1_s, 20_m } } } } },
			};
			auto const codomain = field.codomain (0.25, 5_deg);
			static_assert (std::is_same_v<decltype (codomain), std::optional<Range<si::Length>> const>);
			test_asserts::verify ("interpolated physical slice has a codomain", codomain.has_value());
			test_asserts::verify_equal_with_epsilon ("minimum retains value units", codomain->min(), 12.5_m, 1e-12_m);
			test_asserts::verify_equal_with_epsilon ("maximum retains value units", codomain->max(), 17.5_m, 1e-12_m);

			auto const constant = field.codomain (0.5, 5_deg);
			test_asserts::verify ("constant physical slice has a codomain", constant.has_value());
			test_asserts::verify_equal_with_epsilon ("constant slice minimum is correct", constant->min(), 15_m, 1e-12_m);
			test_asserts::verify_equal_with_epsilon ("constant slice maximum is correct", constant->max(), 15_m, 1e-12_m);
			test_asserts::verify ("invalid first fixed argument is rejected", !field.codomain (2.0, 5_deg));
			test_asserts::verify ("invalid second fixed argument is rejected", !field.codomain (0.5, 11_deg));

			auto const global = field.codomain();
			static_assert (std::is_same_v<decltype (global), Range<si::Length> const>);
			test_asserts::verify_equal ("global minimum retains value units", global.min(), 0_m);
			test_asserts::verify_equal ("global maximum retains value units", global.max(), 30_m);
		});

		it ("selects domain bounds using the fixed first argument", []{
			math::Field<double, double, double> const field {
				{ 0.0, { { 10.0, 1.0 }, { 20.0, 2.0 } } },
				{ 1.0, { { 30.0, 3.0 }, { 40.0, 4.0 } } },
				{ 2.0, { { 50.0, 5.0 }, { 60.0, 6.0 } } },
			};

			for (auto const x: { 0.0, 1.0, 2.0 })
			{
				auto const domain = field.domain (x);
				static_assert (std::is_same_v<decltype (domain), std::optional<Range<double>> const>);
				test_asserts::verify ("domain exists at the fixed grid knot", domain.has_value());
				test_asserts::verify_equal ("minimum belongs to the selected slice", domain->min(), 10.0 + 20.0 * x);
				test_asserts::verify_equal ("maximum belongs to the selected slice", domain->max(), 20.0 + 20.0 * x);
			}
		});

		it ("returns a domain between grid knots and no domain outside the fixed argument range", []{
			math::Field<double, double, double> const field {
				{ 0.0, { { -3.0, 1.0 }, { 7.0, 2.0 } } },
				{ 2.0, { { -3.0, 3.0 }, { 7.0, 4.0 } } },
			};
			auto const domain = field.domain (1.0);
			test_asserts::verify ("domain exists between grid knots", domain.has_value());
			test_asserts::verify_equal ("interior minimum is correct", domain->min(), -3.0);
			test_asserts::verify_equal ("interior maximum is correct", domain->max(), 7.0);
			test_asserts::verify ("fixed argument below the range has no domain", !field.domain (-1.0));
			test_asserts::verify ("fixed argument above the range has no domain", !field.domain (3.0));
		});

		it ("preserves the next argument's physical unit when selecting a 3D field domain", []{
			math::Field<si::Time, si::Length, si::Angle, double> const field {
				{ 0_s, { { 10_m, { { 0_deg, 1.0 }, { 1_deg, 2.0 } } }, { 20_m, { { 0_deg, 3.0 }, { 1_deg, 4.0 } } } } },
				{ 2_s, { { 30_m, { { 0_deg, 5.0 }, { 1_deg, 6.0 } } }, { 40_m, { { 0_deg, 7.0 }, { 1_deg, 8.0 } } } } },
			};
			auto const first = field.domain (0_s);
			auto const last = field.domain (2_s);
			static_assert (std::is_same_v<decltype (first), std::optional<Range<si::Length>> const>);
			test_asserts::verify ("first slice has a length domain", first.has_value());
			test_asserts::verify ("last slice has a length domain", last.has_value());
			test_asserts::verify_equal ("first slice minimum retains length units", first->min(), 10_m);
			test_asserts::verify_equal ("first slice maximum retains length units", first->max(), 20_m);
			test_asserts::verify_equal ("last slice minimum retains length units", last->min(), 30_m);
			test_asserts::verify_equal ("last slice maximum retains length units", last->max(), 40_m);
			test_asserts::verify ("invalid physical fixed argument has no domain", !field.domain (3_s));
			auto const outer = field.domain();
			static_assert (std::is_same_v<decltype (outer), Range<si::Time> const>);
			test_asserts::verify_equal ("unfixed domain retains the first argument's unit", outer.min(), 0_s);
			test_asserts::verify_equal ("unfixed domain retains the first argument's maximum", outer.max(), 2_s);
		});

		it ("interpolates argument bounds using both fixed coordinates of a 3D field", []{
			math::Field<double, si::Angle, si::Time, double> const field {
				{ 0.0, { { 0_deg, { { 0_s, 1.0 }, { 20_s, 2.0 } } }, { 10_deg, { { 4_s, 3.0 }, { 24_s, 4.0 } } } } },
				{ 2.0, { { 0_deg, { { 8_s, 5.0 }, { 28_s, 6.0 } } }, { 10_deg, { { 12_s, 7.0 }, { 32_s, 8.0 } } } } },
			};
			std::array<std::tuple<double, si::Angle, si::Time, si::Time>, 4> const cases {{
				{ 0.0, 0_deg, 0_s, 20_s },
				{ 0.0, 10_deg, 4_s, 24_s },
				{ 1.0, 2.5_deg, 5_s, 25_s },
				{ 2.0, 10_deg, 12_s, 32_s },
			}};

			for (auto const& [x, angle, expected_min, expected_max]: cases)
			{
				auto const min = field.min_argument (x, angle);
				auto const max = field.max_argument (x, angle);
				auto const domain = field.domain (x, angle);
				static_assert (std::is_same_v<decltype (min), std::optional<si::Time> const>);
				static_assert (std::is_same_v<decltype (max), std::optional<si::Time> const>);
				static_assert (std::is_same_v<decltype (domain), std::optional<Range<si::Time>> const>);
				test_asserts::verify ("both argument bounds exist, including zero", min.has_value() && max.has_value());
				test_asserts::verify_equal_with_epsilon ("minimum uses both fixed coordinates", *min, expected_min, 1e-12_s);
				test_asserts::verify_equal_with_epsilon ("maximum uses both fixed coordinates", *max, expected_max, 1e-12_s);
				test_asserts::verify ("domain accepts two fixed arguments", domain.has_value());
				test_asserts::verify_equal_with_epsilon ("domain minimum agrees with the bound", domain->min(), expected_min, 1e-12_s);
				test_asserts::verify_equal_with_epsilon ("domain maximum agrees with the bound", domain->max(), expected_max, 1e-12_s);
			}
		});

		it ("preserves argument units through three levels of bound interpolation in a 4D field", []{
			using Field = math::Field<double, si::Angle, si::Time, si::Length, double>;
			Field::DataMap data;

			for (auto const x: { 0.0, 2.0 })
			{
				for (auto const angle: { 0_deg, 10_deg })
				{
					for (auto const time: { 0_s, 4_s })
					{
						auto const bound = x * 4_m + (angle / 10_deg) * 8_m + (time / 4_s) * 16_m;
						data[x][angle][time][bound] = 0.0;
						data[x][angle][time][bound + 32_m] = 1.0;
					}
				}
			}

			Field const field (std::move (data));
			auto const time_domain = field.domain (0.5, 2.5_deg);
			static_assert (std::is_same_v<decltype (time_domain), std::optional<Range<si::Time>> const>);
			test_asserts::verify ("two fixed arguments leave a time domain", time_domain.has_value());
			test_asserts::verify_equal ("time minimum retains its unit", time_domain->min(), 0_s);
			test_asserts::verify_equal ("time maximum retains its unit", time_domain->max(), 4_s);

			for (auto const time: { 1_s, 3_s })
			{
				auto const min = field.min_argument (0.5, 2.5_deg, time);
				auto const max = field.max_argument (0.5, 2.5_deg, time);
				auto const domain = field.domain (0.5, 2.5_deg, time);
				auto const expected_min = 4_m + (time / 4_s) * 16_m;
				static_assert (std::is_same_v<decltype (min), std::optional<si::Length> const>);
				static_assert (std::is_same_v<decltype (max), std::optional<si::Length> const>);
				static_assert (std::is_same_v<decltype (domain), std::optional<Range<si::Length>> const>);
				test_asserts::verify ("three fixed arguments have length bounds", min.has_value() && max.has_value());
				test_asserts::verify_equal_with_epsilon ("minimum uses every coordinate", *min, expected_min, 1e-12_m);
				test_asserts::verify_equal_with_epsilon ("maximum uses every coordinate", *max, expected_min + 32_m, 1e-12_m);
				test_asserts::verify ("domain accepts three fixed arguments", domain.has_value());
				test_asserts::verify_equal_with_epsilon ("domain minimum retains length units", domain->min(), expected_min, 1e-12_m);
				test_asserts::verify_equal_with_epsilon ("domain maximum retains length units", domain->max(), expected_min + 32_m, 1e-12_m);
			}

			test_asserts::verify ("deepest fixed argument below the domain is rejected", !field.min_argument (0.5, 2.5_deg, -1_s));
			test_asserts::verify ("deepest fixed argument above the domain is rejected", !field.max_argument (0.5, 2.5_deg, 5_s));
			test_asserts::verify ("domain rejects an unavailable deepest slice", !field.domain (0.5, 2.5_deg, 5_s));
		});

		it ("uses only exact slices for argument bounds and rejects missing neighboring slices", []{
			math::Field<double, si::Angle, si::Time, double> const field {
				{ 0.0, { { 0_deg, { { 0_s, 1.0 }, { 1_s, 2.0 } } }, { 10_deg, { { 0_s, 3.0 }, { 1_s, 4.0 } } } } },
				{ 1.0, { { 20_deg, { { 2_s, 5.0 }, { 3_s, 6.0 } } }, { 30_deg, { { 2_s, 7.0 }, { 3_s, 8.0 } } } } },
				{ 2.0, { { 40_deg, { { 4_s, 9.0 }, { 5_s, 10.0 } } }, { 50_deg, { { 4_s, 11.0 }, { 5_s, 12.0 } } } } },
			};

			for (auto const x: { 0.0, 1.0, 2.0 })
			{
				auto const domain = field.domain (x, (5.0 + 20.0 * x) * 1_deg);
				test_asserts::verify ("exact outer knot does not require neighboring slices", domain.has_value());
				test_asserts::verify_equal ("selected slice supplies the minimum", domain->min(), (2.0 * x) * 1_s);
				test_asserts::verify_equal ("selected slice supplies the maximum", domain->max(), (1.0 + 2.0 * x) * 1_s);
			}

			std::array<std::pair<double, si::Angle>, 6> const unavailable {{
				{ -1.0, 5_deg }, { 3.0, 45_deg }, { 0.0, -1_deg }, { 2.0, 51_deg },
				{ 0.5, 5_deg }, { 0.5, 25_deg },
			}};

			for (auto const& [x, angle]: unavailable)
			{
				test_asserts::verify ("missing slice has no minimum argument", !field.min_argument (x, angle));
				test_asserts::verify ("missing slice has no maximum argument", !field.max_argument (x, angle));
				test_asserts::verify ("missing slice has no domain", !field.domain (x, angle));
			}
		});

		it ("returns argument bounds for singleton slices with zero coordinates", []{
			math::Field<si::Time, si::Length, si::Angle, double> const field {
				{ 0_s, { { 0_m, { { 0_deg, 42.0 } } } } },
			};
			auto const length_domain = field.domain (0_s);
			auto const angle_domain = field.domain (0_s, 0_m);
			test_asserts::verify ("singleton first coordinate has a domain", length_domain.has_value());
			test_asserts::verify_equal ("singleton length minimum is zero", length_domain->min(), 0_m);
			test_asserts::verify_equal ("singleton length maximum is zero", length_domain->max(), 0_m);
			test_asserts::verify ("singleton second coordinate has a domain", angle_domain.has_value());
			test_asserts::verify_equal ("singleton angle minimum is zero", angle_domain->min(), 0_deg);
			test_asserts::verify_equal ("singleton angle maximum is zero", angle_domain->max(), 0_deg);
			test_asserts::verify ("unavailable singleton first coordinate is rejected", !field.domain (1_s, 0_m));
			test_asserts::verify ("unavailable singleton second coordinate is rejected", !field.domain (0_s, 1_m));
		});

		it ("restricts a singleton field to its only argument", []{
			math::Field<double, double> const field { { 1.0, 42.0 } };

			test_asserts::verify_equal ("stored value is available", field.value (1.0).value_or (-1.0), 42.0);

			for (auto const x: { 0.0, 2.0 })
			{
				test_asserts::verify ("outside argument is rejected", !field.value (x));
				test_asserts::verify_equal ("explicit extrapolation returns the singleton value", field.extrapolated_value (x), 42.0);
			}
		});

		it ("checks every coordinate of a singleton field with physical quantities", []{
			math::Field<double, si::Angle, si::Time, si::Length> const field {
				{ 0.0, { { 0_deg, { { 1_s, 42_m } } } } },
			};

			test_asserts::verify_equal ("stored point is available", field.value (0.0, 0_deg, 1_s).value_or (-1_m), 42_m);

			for (auto const x: { -1.0, 1.0 })
				test_asserts::verify ("first coordinate outside its singleton domain is rejected", !field.value (x, 0_deg, 1_s));

			for (auto const angle: { -1_deg, 1_deg })
				test_asserts::verify ("middle coordinate outside its singleton domain is rejected", !field.value (0.0, angle, 1_s));

			for (auto const time: { 0_s, 2_s })
				test_asserts::verify ("last coordinate outside its singleton domain is rejected", !field.value (0.0, 0_deg, time));

			test_asserts::verify_equal ("explicit extrapolation handles all singleton coordinates",
										field.extrapolated_value (2.0, 3_deg, 4_s), 42_m);
		});

		it ("checks all interpolation dimensions while preserving constant extrapolation", []{
			using Field = math::Field<double, double, double, double>;
			Field::DataMap data;
			std::array<double, 2> const knots { 0.0, 1.0 };

			for (auto const x: knots)
				for (auto const y: knots)
					for (auto const z: knots)
						data[x][y][z] = 100.0 * x + 10.0 * y + z;

			Field const field (std::move (data));
			std::array<std::array<double, 4>, 6> const cases {{
				{ -1.0, 0.5, 0.5, 5.5 },
				{ 2.0, 0.5, 0.5, 105.5 },
				{ 0.5, -1.0, 0.5, 50.5 },
				{ 0.5, 2.0, 0.5, 60.5 },
				{ 0.5, 0.5, -1.0, 55.0 },
				{ 0.5, 0.5, 2.0, 56.0 },
			}};

			for (auto const& point: cases)
			{
				test_asserts::verify ("outside coordinate is rejected", !field.value (point[0], point[1], point[2]));
				test_asserts::verify_equal_with_epsilon ("explicit extrapolation clamps the outside coordinate",
					field.extrapolated_value (point[0], point[1], point[2]), point[3], 1e-12);
			}

			test_asserts::verify_equal_with_epsilon ("interior interpolation is correct", field.value (0.25, 0.5, 0.75).value_or (-1.0), 30.75, 1e-12);
			test_asserts::verify_equal ("lower boundary is included", field.value (0.0, 0.0, 0.0).value_or (-1.0), 0.0);
			test_asserts::verify_equal ("upper boundary is included", field.value (1.0, 1.0, 1.0).value_or (-1.0), 111.0);
		});

		it ("reads exact knots without requiring neighboring slices to overlap", []{
			math::Field<double, double, double> const field {
				{ 0.0, { { 0.0, 42.0 }, { 1.0, 43.0 } } },
				{ 1.0, { { 2.0, 100.0 }, { 3.0, 200.0 } } },
				{ 2.0, { { 4.0, 400.0 }, { 5.0, 500.0 } } },
			};

			for (auto const& [x, slice]: field.data_map())
				for (auto const& [y, expected]: slice)
					test_asserts::verify_equal ("stored point is available", field.value (x, y).value_or (-1.0), expected);

			test_asserts::verify_equal ("first slice interpolates independently", field.value (0.0, 0.5).value_or (-1.0), 42.5);
			test_asserts::verify_equal ("middle slice interpolates independently", field.value (1.0, 2.5).value_or (-1.0), 150.0);
			test_asserts::verify_equal ("last slice interpolates independently", field.value (2.0, 4.5).value_or (-1.0), 450.0);
			test_asserts::verify ("interpolation requires both adjacent domains", !field.value (0.5, 0.5));
			test_asserts::verify ("interpolation checks the other adjacent domain", !field.value (0.5, 2.5));
			test_asserts::verify_equal ("lower extrapolation selects the first slice", field.extrapolated_value (-1.0, 0.5), 42.5);
			test_asserts::verify_equal ("upper extrapolation selects the last slice", field.extrapolated_value (3.0, 4.5), 450.0);
			verify_extremum (field, field.min_value_point (0.0), 42.0, 1e-12);
			verify_extremum (field, field.max_value_point (1.0), 200.0, 1e-12);
			verify_extremum (field, field.min_value_point (2.0), 400.0, 1e-12);
		});

		it ("does not interpolate singleton slices outside their domains", []{
			math::Field<double, double, double> const field {
				{ 0.0, { { 0.0, 1.0 } } },
				{ 1.0, { { 1.0, 2.0 } } },
			};

			test_asserts::verify_equal ("first singleton slice is available at its knot", field.value (0.0, 0.0).value_or (-1.0), 1.0);
			test_asserts::verify_equal ("last singleton slice is available at its knot", field.value (1.0, 1.0).value_or (-1.0), 2.0);

			for (auto const y: { 0.0, 0.5, 1.0 })
				test_asserts::verify ("disjoint singleton domains cannot be interpolated", !field.value (0.5, y));

			test_asserts::verify_equal ("explicit extrapolation interpolates the constant singleton values", field.extrapolated_value (0.5, 0.5), 1.5);
		});

		it ("finds extrema after interpolating fixed arguments with physical quantities", []{
			math::Field<double, si::Angle, si::Time, si::Length> const field {
				{ 0.0, { { 0_deg, { { 0_s, 0_m }, { 1_s, 10_m } } } } },
				{ 1.0, { { 0_deg, { { 0_s, 10_m }, { 1_s, 0_m } } } } },
			};

			for (auto const& point: { field.min_value_point (0.25), field.min_value_point (0.25, 0_deg) })
			{
				verify_extremum (field, point, 2.5_m, 1e-12_m);
				test_asserts::verify_equal ("minimum keeps the fixed first argument", std::get<0> (point->arguments), 0.25);
				test_asserts::verify_equal ("minimum is at the first time knot", std::get<2> (point->arguments), 0_s);
			}

			for (auto const& point: { field.max_value_point (0.25), field.max_value_point (0.25, 0_deg) })
			{
				verify_extremum (field, point, 7.5_m, 1e-12_m);
				test_asserts::verify_equal ("maximum keeps the fixed first argument", std::get<0> (point->arguments), 0.25);
				test_asserts::verify_equal ("maximum is at the last time knot", std::get<2> (point->arguments), 1_s);
			}

			verify_extremum (field, field.min_value_point (0.5, 0_deg), 5_m, 1e-12_m);
			verify_extremum (field, field.max_value_point (0.5, 0_deg), 5_m, 1e-12_m);
			test_asserts::verify_equal_with_epsilon ("min_value uses the interpolated field", *field.min_value (0.25, 0_deg), 2.5_m, 1e-12_m);
			test_asserts::verify_equal_with_epsilon ("max_value uses the interpolated field", *field.max_value (0.25, 0_deg), 7.5_m, 1e-12_m);
		});

		it ("searches both free arguments at the same point across slices", []{
			math::Field<double, double, double, double> const field {
				{ 0.0, { { 0.0, { { 0.0, 0.0 }, { 1.0, 10.0 } } }, { 1.0, { { 0.0, 20.0 }, { 1.0, 30.0 } } } } },
				{ 1.0, { { 0.0, { { 0.0, 30.0 }, { 1.0, 20.0 } } }, { 1.0, { { 0.0, 10.0 }, { 1.0, 0.0 } } } } },
			};

			verify_extremum (field, field.min_value_point (0.5), 15.0, 1e-12);
			verify_extremum (field, field.max_value_point (0.5), 15.0, 1e-12);
		});

		it ("searches knots present in only one of the interpolated grids", []{
			math::Field<double, double, double> const field {
				{ 0.0, { { 0.0, 0.0 }, { 0.5, -12.0 }, { 2.0, 0.0 } } },
				{ 1.0, { { 0.0, 0.0 }, { 1.5, 12.0 }, { 2.0, 0.0 } } },
			};
			auto const minimum = field.min_value_point (0.5);
			auto const maximum = field.max_value_point (0.5);

			verify_extremum (field, minimum, -4.0, 1e-12);
			verify_extremum (field, maximum, 4.0, 1e-12);
			test_asserts::verify_equal ("minimum is at the first grid's interior knot", std::get<1> (minimum->arguments), 0.5);
			test_asserts::verify_equal ("maximum is at the second grid's interior knot", std::get<1> (maximum->arguments), 1.5);
		});

		it ("finds extrema with two fixed and two free arguments", []{
			using Field = math::Field<double, double, double, double, double>;
			Field::DataMap data;
			std::array<double, 2> const knots { 0.0, 1.0 };

			for (auto const x: knots)
				for (auto const y: knots)
					for (auto const z: knots)
						for (auto const w: knots)
							data[x][y][z][w] = (x == 0.0 ? 10.0 * w : 10.0 * (1.0 - w)) +
								(y == 0.0 ? 20.0 * z : 20.0 * (1.0 - z));

			Field const field (std::move (data));
			auto const minimum = field.min_value_point (0.25, 0.75);
			auto const maximum = field.max_value_point (0.25, 0.75);

			verify_extremum (field, minimum, 7.5, 1e-12);
			verify_extremum (field, maximum, 22.5, 1e-12);
			test_asserts::verify_equal ("minimum has the expected arguments", minimum->arguments, std::make_tuple (0.25, 0.75, 1.0, 0.0));
			test_asserts::verify_equal ("maximum has the expected arguments", maximum->arguments, std::make_tuple (0.25, 0.75, 0.0, 1.0));
		});

		it ("returns no extremum when interpolated slices have disjoint domains", []{
			math::Field<double, double, double> const field {
				{ 0.0, { { 0.0, 1.0 }, { 1.0, 2.0 } } },
				{ 1.0, { { 2.0, 3.0 }, { 3.0, 4.0 } } },
			};

			test_asserts::verify ("minimum is undefined", !field.min_value_point (0.5));
			test_asserts::verify ("maximum is undefined", !field.max_value_point (0.5));
		});

		it ("rejects fixed arguments outside the field domain when searching for extrema", []{
			math::Field<double, double, double, double> const field {
				{ 0.0, { { 0.0, { { 0.0, 1.0 }, { 1.0, 2.0 } } }, { 1.0, { { 0.0, 3.0 }, { 1.0, 4.0 } } } } },
				{ 1.0, { { 0.0, { { 0.0, 5.0 }, { 1.0, 6.0 } } }, { 1.0, { { 0.0, 7.0 }, { 1.0, 8.0 } } } } },
			};

			test_asserts::verify ("minimum rejects the first argument below the domain", !field.min_value_point (-1.0));
			test_asserts::verify ("maximum rejects the first argument above the domain", !field.max_value_point (2.0));
			test_asserts::verify ("minimum rejects the second argument below the domain", !field.min_value_point (0.5, -1.0));
			test_asserts::verify ("maximum rejects the second argument above the domain", !field.max_value_point (0.5, 2.0));
		});

		it ("1 argument, 1 value", []{
			math::Field<double, double> field {
				{ 0.0, 0.0 },
				{ 1.0, 10.0 },
				{ 0.0, 0.0 },
				{ 2.0, 20.0 },
				{ 3.0, 10.0 },
				{ 4.0, 0.0 },
			};

			test_asserts::verify ("domain().min() is correct", field.domain().min() == 0.0);
			test_asserts::verify ("domain().max() is correct", field.domain().max() == 4.0);

			test_asserts::verify ("codomain().min() is correct", field.codomain().min() == 0.0);
			test_asserts::verify ("codomain().max() is correct", field.codomain().max() == 20.0);
		});

		it ("2 arguments, 1 value", []{
			math::Field<double, si::Angle, si::Time> field {
				{
					0.0,
					{
						{ 0.0_deg, 0.0_s },
						{ 1.0_deg, 10.0_s },
						{ 2.0_deg, 20.0_s },
						{ 3.0_deg, 30.0_s },
						// 4.0_deg would extrapolate to 30.0_s.
					},
				},
				{
					1.0,
					{
						// 0.0_deg would extrapolate to 100.0_s.
						{ 1.0_deg, 100.0_s },
						{ 2.0_deg, 200.0_s },
						{ 3.0_deg, 300.0_s },
						{ 4.0_deg, 400.0_s },
					},
				}
			};

			test_asserts::verify_equal_with_epsilon ("min_argument() == 0.0", field.min_argument(), 0.0, 0.000001);
			test_asserts::verify_equal_with_epsilon ("max_argument() == 1.0", field.max_argument(), 1.0, 0.000001);

			test_asserts::verify ("min_argument (-1.0) == null", !field.min_argument (-1.0));
			test_asserts::verify ("max_argument (-1.0) == null", !field.max_argument (-1.0));

			test_asserts::verify_equal_with_epsilon ("min_argument (0.0) == 0°", field.min_argument (0.0).value_or (-1_deg), 0_deg, 0.000001_deg);
			test_asserts::verify_equal_with_epsilon ("max_argument (0.0) == 3°", field.max_argument (0.0).value_or (-1_deg), 3_deg, 0.000001_deg);

			test_asserts::verify_equal_with_epsilon ("min_argument (1.0) == 1°", field.min_argument (1.0).value_or (-1_deg), 1_deg, 0.000001_deg);
			test_asserts::verify_equal_with_epsilon ("max_argument (1.0) == 4°", field.max_argument (1.0).value_or (-1_deg), 4_deg, 0.000001_deg);

			test_asserts::verify ("returned value for outside of domain is std::nullopt", !field.value (100.0, 0.0_deg));

			test_asserts::verify_equal_with_epsilon ("value (0.0, 1.0°) == 0 s", field.value (0.0, 1.0_deg).value_or (-1.0_s), 10_s, 0.000001_s);
			test_asserts::verify_equal_with_epsilon ("value (0.0, 3.0°) == 30 s", field.value (0.0, 3.0_deg).value_or (-1.0_s), 30_s, 0.000001_s);

			test_asserts::verify_equal_with_epsilon ("value (1.0, 1.0°) == 100 s", field.value (1.0, 1.0_deg).value_or (-1.0_s), 100_s, 0.000001_s);
			test_asserts::verify_equal_with_epsilon ("value (1.0, 3.0°) == 300 s", field.value (1.0, 3.0_deg).value_or (-1.0_s), 300_s, 0.000001_s);

			test_asserts::verify_equal_with_epsilon ("extrapolated_value (0.0, 0°) == 0 s", field.extrapolated_value (0.0, 0.0_deg), 0_s, 0.000001_s);
			test_asserts::verify_equal_with_epsilon ("extrapolated_value (1.0, 0°) == 100 s", field.extrapolated_value (1.0, 0.0_deg), 100_s, 0.000001_s);

			test_asserts::verify_equal_with_epsilon ("extrapolated_value (0.5, 0.0°) == 50 s", field.extrapolated_value (0.5, 0.0_deg), 50_s, 0.000001_s);
			test_asserts::verify_equal_with_epsilon ("extrapolated_value (0.5, 2.0°) == 110 s", field.extrapolated_value (0.5, 2.0_deg), 110_s, 0.000001_s);
			test_asserts::verify_equal_with_epsilon ("extrapolated_value (0.5, 4.0°) == 215 s", field.extrapolated_value (0.5, 4.0_deg), 215_s, 0.000001_s);
			test_asserts::verify_equal_with_epsilon ("operator[] (0.5, 2.0°) == 110 s", field[0.5, 2.0_deg], 110_s, 0.000001_s);

			// 2D search:
			test_asserts::verify_equal_with_epsilon ("min_value() == 0_s", field.min_value(), 0.0_s, 0.000001_s);

			// 1D search:
			test_asserts::verify_equal_with_epsilon ("min_value (0.0) == 0_s", field.min_value (0.0).value_or (-1_s), 0_s, 0.000001_s);
			test_asserts::verify_equal_with_epsilon ("min_value (0.5) == 50_s", field.min_value (0.5).value_or (-1_s), 55_s, 0.000001_s);
			test_asserts::verify_equal_with_epsilon ("min_value (1.0) == 100_s", field.min_value (1.0).value_or (-1_s), 100_s, 0.000001_s);

			// Ensure arguments are {0.0, 0.0_deg}:
			{
				auto const point = field.min_value_point (0.0);
				test_asserts::verify ("value is inside domain", !!point);
				test_asserts::verify_equal_with_epsilon ("min_value_point (0.0) is at argument<0> 0.0", std::get<0> (point->arguments), 0.0, 0.000001);
				test_asserts::verify_equal_with_epsilon ("min_value_point (0.0) is at argument<1> 0_deg", std::get<1> (point->arguments), 0_deg, 0.000001_deg);
			}

			// Test if arguments are properly renormalized:
			{
				auto const point = field.min_value_point (0.5);
				test_asserts::verify ("value is inside domain", !!point);
				test_asserts::verify_equal_with_epsilon ("min_value_point (0.5) is at argument<0> 0.5", std::get<0> (point->arguments), 0.5, 0.000001);
				test_asserts::verify_equal_with_epsilon ("min_value_point (0.5) is at argument<1> 1_deg", std::get<1> (point->arguments), 1_deg, 0.000001_deg);
			}
		});

		it ("3 arguments, 1 value", []{
			math::Field<double, si::Angle, si::Time, si::Length> field {
				{
					0.0,
					{
						{
							0.0_deg,
							{
								{ 1.0_s, 10.0_m },
								{ 2.0_s, 20.0_m },
								{ 3.0_s, 30.0_m },
							},
						},
						{
							10.0_deg,
							{
								{ 1.0_s, -10.0_m },
								{ 2.0_s, -20.0_m },
								{ 3.0_s, -30.0_m },
							},
						},
					},
				},
				{
					1.0,
					{
						{
							0.0_deg,
							{
								{ 2.0_s, 200.0_m },
								{ 3.0_s, 300.0_m },
								{ 4.0_s, 400.0_m },
							},
						},
						{
							10.0_deg,
							{
								{ 2.0_s, -200.0_m },
								{ 3.0_s, -300.0_m },
								{ 4.0_s, -400.0_m },
							},
						},
					},
				},
				{
					2.0,
					{
						{
							0.0_deg,
							{
								{ 0.0_s, -100.0_m },
								{ 1.0_s, -70.0_m },
								{ 2.0_s, 0.0_m },
							},
						},
						{
							10.0_deg,
							{
								{ 0.0_s, 0.0_m },
								{ 1.0_s, -70.0_m },
								{ 2.0_s, -100.0_m },
							},
						},
					},
				},
			};

			// 3D search:
			test_asserts::verify_equal_with_epsilon ("min_value() == -400_m", field.min_value(), -400_m, 0.000001_m);

			// 2D search:
			test_asserts::verify_equal_with_epsilon ("min_value (0.0) == -30_m", field.min_value (0.0).value_or (-1_m), -30_m, 0.000001_m);
			test_asserts::verify_equal_with_epsilon ("min_value (0.5) == -165_m", field.min_value (0.5).value_or (-1_m), -165_m, 0.000001_m);

			// 1D search:
			// At exact x knots, only that slice determines the available time domain.
			test_asserts::verify_equal_with_epsilon ("min_value (0.0, 0°) == 10_m", field.min_value (0.0, 0_deg).value_or (-1_m), 10_m, 0.000001_m);
			test_asserts::verify_equal_with_epsilon ("min_value (0.0, 5°) == 0_m", field.min_value (0.0, 5_deg).value_or (-1_m), 0_m, 0.000001_m);
			test_asserts::verify_equal_with_epsilon ("min_value (0.0, 10°) == -30_m", field.min_value (0.0, 10_deg).value_or (-1_m), -30_m, 0.000001_m);
			test_asserts::verify_equal_with_epsilon ("min_value (0.5, 0°) == 110_m", field.min_value (0.5, 0_deg).value_or (-1_m), 110_m, 0.000001_m);
			test_asserts::verify_equal_with_epsilon ("min_value (1.0, 0°) == 200_m", field.min_value (1.0, 0_deg).value_or (-1_m), 200_m, 0.000001_m);
			test_asserts::verify_equal_with_epsilon ("min_value (1.0, 10°) == -400_m", field.min_value (1.0, 10_deg).value_or (-1_m), -400_m, 0.000001_m);
			test_asserts::verify_equal_with_epsilon ("min_value (2.0, 0°) == -100_m", field.min_value (2.0, 0_deg).value_or (-1_m), -100_m, 0.000001_m);
			test_asserts::verify_equal_with_epsilon ("min_value (2.0, 5°) == -70_m", field.min_value (2.0, 5_deg).value_or (-1_m), -70_m, 0.000001_m);
			test_asserts::verify_equal_with_epsilon ("min_value (2.0, 10°) == -100_m", field.min_value (2.0, 10_deg).value_or (-1_m), -100_m, 0.000001_m);

			for (auto const x: { 0.0, 0.5, 1.0, 2.0 })
			{
				for (auto const angle: { 0_deg, 5_deg, 10_deg })
				{
					auto const minimum = field.min_value_point (x, angle);
					auto const maximum = field.max_value_point (x, angle);

					verify_extremum (field, minimum, *field.min_value (x, angle), 1e-12_m);
					verify_extremum (field, maximum, *field.max_value (x, angle), 1e-12_m);
				}
			}

			for (auto const& [x, angle_slices]: field.data_map())
				for (auto const& [angle, time_slice]: angle_slices)
					for (auto const& [time, expected]: time_slice)
						test_asserts::verify_equal ("stored point is available with physical quantities", field.value (x, angle, time).value_or (1000_m), expected);
		});

		it ("N arguments..., 1 value", []{
			math::Field<double, si::Angle, si::Time, si::Length, si::Mass, si::Amount> field {
				{ 0.5, { { 1_rad, { { 1_s, { { 1_m, { { 1_kg, 1_mol } } } } } } } } },
			};

			test_asserts::verify_equal_with_epsilon ("min_value() is 1_mol", field.min_value(), 1_mol, 0.000001_mol);
			test_asserts::verify_equal_with_epsilon ("min_value (0.5) is 1_mol", field.min_value (0.5).value_or (-1_mol), 1_mol, 0.000001_mol);
			test_asserts::verify_equal_with_epsilon ("min_value (0.5, 1_rad) is 1_mol", field.min_value (0.5, 1_rad).value_or (-1_mol), 1_mol, 0.000001_mol);
			test_asserts::verify_equal_with_epsilon ("min_value (0.5, 1_rad, 1_s) is 1_mol", field.min_value (0.5, 1_rad, 1_s).value_or (-1_mol), 1_mol, 0.000001_mol);
			test_asserts::verify_equal_with_epsilon ("min_value (0.5, 1_rad, 1_s, 1_m) is 1_mol", field.min_value (0.5, 1_rad, 1_s, 1_m).value_or (-1_mol), 1_mol, 0.000001_mol);
		});

		it ("Vector value", []{
			using Vector = math::Vector<double, 3>;

			math::Field<double, Vector> field {
				{
					0.0,
					Vector { 1.0, 0.0, 0.0 },
				},
				{
					1.0,
					Vector { 0.0, 1.0, 0.0 },
				},
			};

			auto const epsilon = 0.000001 * Vector { 1.0, 1.0, 1.0 };

			test_asserts::verify_equal_with_epsilon ("field.value (0.5) is vector { 0.5, 0.5, 0.0 }", abs (*field.value (0.5)), abs (Vector { 0.5, 0.5, 0.0 }), abs (epsilon));
		});
	});
});

} // namespace
} // namespace neutrino::test

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

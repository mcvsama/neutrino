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
#include <neutrino/math/math.h>
#include <neutrino/si/si.h>
#include <neutrino/test/test_asserts.h>
#include <neutrino/test/test_suite.h>

// Standard:
#include <array>
#include <cmath>
#include <concepts>
#include <limits>
#include <type_traits>


namespace neutrino::test {
namespace {

template<math::Scalar S>
	static void
	verify_norm (S const actual, S const expected)
	{
		using Value = si::decay_quantity_t<S>;
		test_asserts::verify ("norm is finite and nonzero", std::isfinite (si::value (actual)) && si::value (actual) > Value (0));
		test_asserts::verify_equal_with_epsilon ("relative norm error is small", actual / expected, Value (1), Value (32) * std::numeric_limits<Value>::epsilon());
	}


template<math::Scalar S>
	static void
	verify_norms_at_scale (S const scale)
	{
		math::Vector<S, 2> const vector2 { -3 * scale, 4 * scale };
		math::Vector<S, 3> const vector3 { -3 * scale, S (0), 4 * scale };
		math::Vector<S, 9> vector9;
		vector9.components().fill (scale);
		math::Quaternion<S> const quaternion { scale, -scale, scale, -scale };
		math::Matrix<S, 3, 2> const matrix { -3 * scale, S (0), S (0), S (0), 4 * scale, S (0) };
		static_assert (std::is_same_v<decltype (vector2.norm()), S>);
		static_assert (std::is_same_v<decltype (quaternion.norm()), S>);
		static_assert (std::is_same_v<decltype (math::euclidean_norm (matrix)), S>);

		verify_norm (vector2.norm(), 5 * scale);
		verify_norm (vector3.norm(), 5 * scale);
		verify_norm (vector9.norm(), 3 * scale);
		verify_norm (quaternion.norm(), 2 * scale);
		verify_norm (math::euclidean_norm (matrix), 5 * scale);
	}


template<math::Scalar S>
	static void
	verify_subnormal_norms()
	{
		using Value = si::decay_quantity_t<S>;
		auto const tiny = S (std::numeric_limits<Value>::denorm_min());
		math::Vector<S, 9> vector;
		vector.components().fill (tiny);
		math::Quaternion<S> const quaternion { tiny, tiny, tiny, tiny };
		math::SquareMatrix<S, 2> const matrix { tiny, tiny, tiny, tiny };

		test_asserts::verify_equal ("nine subnormal vector components give three times their magnitude", vector.norm(), 3 * tiny);
		test_asserts::verify_equal ("four subnormal quaternion components give twice their magnitude", quaternion.norm(), 2 * tiny);
		test_asserts::verify_equal ("four subnormal matrix components give twice their magnitude", math::euclidean_norm (matrix), 2 * tiny);
	}


static TestRegistration tests ([]{
	describe ("Neutrino", "math", "normalization", []{
		it ("avoids overflow and underflow for float, double and long double components", []{
			auto const verify_type = []<std::floating_point Value> {
				for (auto const scale: { Value (1), std::numeric_limits<Value>::max() / Value (8), std::numeric_limits<Value>::min() * Value (8) })
					verify_norms_at_scale (scale);
			};

			verify_type.operator()<float>();
			verify_type.operator()<double>();
			verify_type.operator()<long double>();
		});

		it ("preserves quantity units and numerical types at extreme magnitudes", []{
			auto const verify_type = []<si::UnitConcept Unit, std::floating_point Value> {
				using Quantity = si::Quantity<Unit, Value>;

				for (auto const scale: { Value (1), std::numeric_limits<Value>::max() / Value (8), std::numeric_limits<Value>::min() * Value (8) })
					verify_norms_at_scale (Quantity (scale));
			};

			verify_type.operator()<si::Meter, float>();
			verify_type.operator()<si::Meter, double>();
			verify_type.operator()<si::Meter, long double>();
			verify_type.operator()<si::Kilometer, float>();
			verify_type.operator()<si::Kilometer, double>();
			verify_type.operator()<si::Kilometer, long double>();
		});

		it ("accumulates subnormal components without repeatedly rounding away their contributions", []{
			verify_subnormal_norms<float>();
			verify_subnormal_norms<double>();
			verify_subnormal_norms<long double>();
			verify_subnormal_norms<si::Quantity<si::Meter, float>>();
			verify_subnormal_norms<si::Quantity<si::Meter, double>>();
			verify_subnormal_norms<si::Quantity<si::Kilometer, double>>();
		});

		it ("handles mixed component magnitudes", []{
			auto const largest = std::numeric_limits<double>::max();
			auto const smallest = std::numeric_limits<double>::denorm_min();
			std::array<double, 4> const components { -largest, smallest, 0.0, 1.0 };
			math::Vector<double, 4> const vector (components);
			math::Quaternion<double> const quaternion (components);
			math::SquareMatrix<double, 2> const matrix (components);

			test_asserts::verify_equal ("vector norm is dominated by the largest component", vector.norm(), largest);
			test_asserts::verify_equal ("quaternion norm is dominated by the largest component", quaternion.norm(), largest);
			test_asserts::verify_equal ("matrix norm is dominated by the largest component", math::euclidean_norm (matrix), largest);
		});

		it ("returns positive zero for zero components", []{
			std::array<double, 4> const components { 0.0, -0.0, 0.0, -0.0 };
			math::Vector<double, 4> const vector (components);
			math::Quaternion<double> const quaternion (components);
			math::SquareMatrix<double, 2> const matrix (components);

			for (auto const norm: { vector.norm(), quaternion.norm(), math::euclidean_norm (matrix) })
			{
				test_asserts::verify_equal ("norm is zero", norm, 0.0);
				test_asserts::verify ("norm is positive zero", !std::signbit (norm));
			}
		});

		it ("handles infinity and NaN consistently with binary hypot", []{
			auto const inf = std::numeric_limits<double>::infinity();
			auto const nan = std::numeric_limits<double>::quiet_NaN();
			std::array<std::array<double, 4>, 4> const infinite_components {{
				{ nan, 1.0, inf, 0.0 },
				{ -inf, nan, 0.0, 1.0 },
				{ 1.0, inf, nan, 0.0 },
				{ 1.0, 0.0, 2.0, -inf },
			}};

			for (auto const& components: infinite_components)
			{
				math::Vector<double, 4> const vector (components);
				math::Quaternion<double> const quaternion (components);
				math::SquareMatrix<double, 2> const matrix (components);

				for (auto const norm: { vector.norm(), quaternion.norm(), math::euclidean_norm (matrix) })
					test_asserts::verify_equal ("infinite component gives a positive infinite norm", norm, inf);
			}

			for (auto const& components: { std::array { nan, 1.0, 2.0, 0.0 }, std::array { 1.0, 0.0, nan, 2.0 } })
			{
				math::Vector<double, 4> const vector (components);
				math::Quaternion<double> const quaternion (components);
				math::SquareMatrix<double, 2> const matrix (components);

				for (auto const norm: { vector.norm(), quaternion.norm(), math::euclidean_norm (matrix) })
					test_asserts::verify ("NaN without infinity propagates", std::isnan (norm));
			}
		});

		it ("returns infinity when the norm itself exceeds the numerical range", []{
			auto const maximum = std::numeric_limits<double>::max();
			std::array<double, 4> const components { maximum, maximum, maximum, maximum };
			math::Vector<double, 4> const vector (components);
			math::Quaternion<double> const quaternion (components);
			math::SquareMatrix<double, 2> const matrix (components);

			for (auto const norm: { vector.norm(), quaternion.norm(), math::euclidean_norm (matrix) })
				test_asserts::verify_equal ("unrepresentable norm is positive infinity", norm, std::numeric_limits<double>::infinity());
		});

		it ("normalizes vectors and quaternions whose component squares overflow or underflow", []{
			for (auto const scale: { std::numeric_limits<double>::max() / 8.0, std::numeric_limits<double>::min() * 8.0 })
			{
				math::Vector<double, 2> vector { 3.0 * scale, 4.0 * scale };
				math::Vector<double, 2> const expected_vector { 0.6, 0.8 };
				math::Quaternion<double> quaternion { scale, scale, scale, scale };
				math::Quaternion<double> const expected_quaternion { 0.5, 0.5, 0.5, 0.5 };

				test_asserts::verify_equal_with_epsilon ("normalized vector copy is correct", vector.normalized(), expected_vector, 1e-12);
				test_asserts::verify_equal_with_epsilon ("normalized quaternion copy is correct", quaternion.normalized(), expected_quaternion, 1e-12);
				vector.normalize();
				quaternion.normalize();
				test_asserts::verify_equal_with_epsilon ("in-place vector normalization is correct", vector, expected_vector, 1e-12);
				test_asserts::verify_equal_with_epsilon ("in-place quaternion normalization is correct", quaternion, expected_quaternion, 1e-12);
			}
		});
	});
});

} // namespace
} // namespace neutrino::test

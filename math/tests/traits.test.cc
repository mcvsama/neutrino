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
#include <neutrino/test/test_asserts.h>
#include <neutrino/test/test_suite.h>

// Standard:
#include <concepts>
#include <type_traits>


namespace neutrino::test {
namespace {

template<class Value>
	concept HasTraitsZero = requires { math::Traits<Value>::zero(); };


static TestRegistration tests ([]{
	describe ("Neutrino", "math", "Traits", []{
		it ("inverts floating-point scalars through the generic function and traits", []{
			auto const verify_type = []<std::floating_point Value> {
				static_assert (std::is_same_v<decltype (math::inverted (Value (4))), Value>);
				static_assert (std::is_same_v<decltype (math::Traits<Value>::inverted (Value (4))), Value>);
				test_asserts::verify_equal ("generic inversion gives the reciprocal", math::inverted (Value (4)), Value (0.25));
				test_asserts::verify_equal ("traits inversion gives the reciprocal", math::Traits<Value>::inverted (Value (4)), Value (0.25));
			};

			verify_type.operator()<float>();
			verify_type.operator()<double>();
			verify_type.operator()<long double>();
		});

		it ("inverts general quaternions through the generic function and traits", []{
			auto const verify = []<std::floating_point Scalar> {
				struct Target: math::CoordinateSystemBase { };
				struct Source: math::CoordinateSystemBase { };
				using Q = math::Quaternion<Scalar, Target, Source>;
				using Inverse = math::Quaternion<Scalar, Source, Target>;

				constexpr Q quaternion { Scalar (1), Scalar (1), Scalar (-1), Scalar (-1) };
				constexpr Inverse expected { Scalar (0.25), Scalar (-0.25), Scalar (0.25), Scalar (0.25) };
				auto const inverse = math::inverted (quaternion);
				static_assert (std::same_as<decltype (inverse), Inverse const>);
				static_assert (std::same_as<decltype (math::Traits<Q>::inverted (quaternion)), Inverse>);
				static_assert (math::Traits<Q>::inverted (quaternion) == expected);
				static_assert (math::Traits<Q>::zero() == Q (math::zero));
				static_assert (math::Traits<Q>::one() == Q (math::identity));
				test_asserts::verify_equal ("generic inversion gives the expected quaternion", inverse, expected);
				test_asserts::verify_equal ("traits inversion gives the expected quaternion", math::Traits<Q>::inverted (quaternion), expected);
				test_asserts::verify_equal ("member and generic inversion agree", inverse, quaternion.inverted());
				test_asserts::verify_equal ("inv and generic inversion agree", inverse, math::inv (quaternion));
				test_asserts::verify_equal ("right inverse gives target-space identity", quaternion * inverse, math::Quaternion<Scalar, Target, Target> (math::identity));
				test_asserts::verify_equal ("left inverse gives source-space identity", inverse * quaternion, math::Quaternion<Scalar, Source, Source> (math::identity));
			};

			verify.operator()<float>();
			verify.operator()<double>();
			verify.operator()<long double>();
		});

		it ("preserves rotation flags and reversed coordinate tags in generic quaternion inversion", []{
			auto const verify = []<std::floating_point Scalar> {
				struct Target: math::CoordinateSystemBase { };
				struct Source: math::CoordinateSystemBase { };
				using Q = math::Quaternion<Scalar, Target, Source, true>;
				using Inverse = math::Quaternion<Scalar, Source, Target, true>;

				constexpr Q quaternion { Scalar (0.5), Scalar (-0.5), Scalar (0.5), Scalar (-0.5) };
				constexpr Inverse expected { Scalar (0.5), Scalar (0.5), Scalar (-0.5), Scalar (0.5) };
				auto const inverse = math::inverted (quaternion);
				static_assert (std::same_as<decltype (inverse), Inverse const>);
				static_assert (math::Traits<Q>::inverted (quaternion) == expected);
				static_assert (math::Traits<Q>::one() == Q (math::identity));
				static_assert (!HasTraitsZero<Q>);
				test_asserts::verify_equal ("generic rotation inversion gives the conjugate", inverse, expected);
				test_asserts::verify_equal ("right inverse remains a rotation", quaternion * inverse, math::Quaternion<Scalar, Target, Target, true> (math::identity));
				test_asserts::verify_equal ("left inverse remains a rotation", inverse * quaternion, math::Quaternion<Scalar, Source, Source, true> (math::identity));
			};

			verify.operator()<float>();
			verify.operator()<double>();
			verify.operator()<long double>();
		});

		it ("preserves reciprocal units in generic physical quaternion inversion", []{
			auto const verify = []<class Scalar, bool IsRotation> {
				struct Target: math::CoordinateSystemBase { };
				struct Source: math::CoordinateSystemBase { };
				using Q = math::Quaternion<Scalar, Target, Source, IsRotation>;
				using InverseScalar = decltype (1 / Scalar (1));
				using Inverse = math::Quaternion<InverseScalar, Source, Target, IsRotation>;
				using Value = si::decay_quantity_t<Scalar>;

				auto const component = IsRotation ? Value (0.5) : Value (1);
				auto const reciprocal = IsRotation ? Value (0.5) : Value (0.25);
				Q const quaternion { Scalar (component), Scalar (component), Scalar (-component), Scalar (-component) };
				Inverse const expected { InverseScalar (reciprocal), InverseScalar (-reciprocal), InverseScalar (reciprocal), InverseScalar (reciprocal) };
				auto const inverse = math::inverted (quaternion);
				static_assert (std::same_as<decltype (inverse), Inverse const>);
				static_assert (std::same_as<decltype (math::Traits<Q>::inverted (quaternion)), Inverse>);
				test_asserts::verify_equal ("generic inversion retains reciprocal units", inverse, expected);
				test_asserts::verify_equal ("traits inversion retains reciprocal units", math::Traits<Q>::inverted (quaternion), expected);
				test_asserts::verify_equal ("physical right inverse gives dimensionless identity", quaternion * inverse, math::Quaternion<Value, Target, Target, IsRotation> (math::identity));
				test_asserts::verify_equal ("physical left inverse gives dimensionless identity", inverse * quaternion, math::Quaternion<Value, Source, Source, IsRotation> (math::identity));
			};

			verify.operator()<si::Quantity<si::Meter, float>, false>();
			verify.operator()<si::Quantity<si::Meter, double>, false>();
			verify.operator()<si::Quantity<si::Meter, long double>, false>();
			verify.operator()<si::Quantity<si::Kilometer, double>, false>();
			verify.operator()<si::Quantity<si::Meter, float>, true>();
			verify.operator()<si::Quantity<si::Meter, double>, true>();
			verify.operator()<si::Quantity<si::Meter, long double>, true>();
			verify.operator()<si::Quantity<si::Kilometer, double>, true>();
		});

		it ("inverts matrices through the generic function and traits", []{
			using Matrix = math::SquareMatrix<double, 2>;
			Matrix const matrix { 4.0, 7.0, 2.0, 6.0 };
			Matrix const expected { 0.6, -0.7, -0.2, 0.4 };
			auto const inverse = math::inverted (matrix);
			static_assert (std::is_same_v<decltype (inverse), Matrix const>);

			test_asserts::verify_equal_with_epsilon ("generic inversion gives the expected matrix", inverse, expected, 1e-12);
			test_asserts::verify_equal_with_epsilon ("traits inversion gives the expected matrix", math::Traits<Matrix>::inverted (matrix), expected, 1e-12);
			test_asserts::verify_equal_with_epsilon ("right inverse gives identity", matrix * inverse, Matrix (math::identity), 1e-12);
			test_asserts::verify_equal_with_epsilon ("left inverse gives identity", inverse * matrix, Matrix (math::identity), 1e-12);
		});

		it ("preserves promoted scalar types and reversed coordinate tags in matrix inverses", []{
			struct Target: math::CoordinateSystemBase { };
			struct Source: math::CoordinateSystemBase { };
			using Matrix = math::SquareMatrix<float, 2, Target, Source>;
			using Inverse = math::SquareMatrix<double, 2, Source, Target>;
			Matrix const matrix { 2.0f, 1.0f, 0.0f, 4.0f };
			Inverse const expected { 0.5, -0.125, 0.0, 0.25 };
			auto const inverse = math::inverted (matrix);
			static_assert (std::is_same_v<decltype (inverse), Inverse const>);
			static_assert (std::is_same_v<decltype (math::Traits<Matrix>::inverted (matrix)), Inverse>);

			test_asserts::verify_equal ("generic inversion retains promoted components", inverse, expected);
			test_asserts::verify_equal ("traits inversion retains promoted components", math::Traits<Matrix>::inverted (matrix), expected);
			test_asserts::verify_equal ("right inverse gives target-space identity", matrix * inverse, math::SquareMatrix<double, 2, Target> (math::identity));
			test_asserts::verify_equal ("left inverse gives source-space identity", inverse * matrix, math::SquareMatrix<double, 2, Source> (math::identity));
		});

		it ("preserves reciprocal physical units and reversed coordinate tags in matrix inverses", []{
			using namespace si::literals;
			struct Target: math::CoordinateSystemBase { };
			struct Source: math::CoordinateSystemBase { };
			using Matrix = math::SquareMatrix<si::Length, 2, Target, Source>;
			using Inverse = math::SquareMatrix<decltype (1.0 / 1_m), 2, Source, Target>;
			Matrix const matrix { 2_m, 1_m, 0_m, 4_m };
			Inverse const expected { 0.5 / 1_m, -0.125 / 1_m, 0.0 / 1_m, 0.25 / 1_m };
			auto const inverse = math::inverted (matrix);
			static_assert (std::is_same_v<decltype (inverse), Inverse const>);
			static_assert (std::is_same_v<decltype (math::Traits<Matrix>::inverted (matrix)), Inverse>);

			test_asserts::verify_equal ("generic inversion retains reciprocal units", inverse, expected);
			test_asserts::verify_equal ("traits inversion retains reciprocal units", math::Traits<Matrix>::inverted (matrix), expected);
			test_asserts::verify_equal ("right inverse gives target-space identity", matrix * inverse, math::SquareMatrix<double, 2, Target> (math::identity));
			test_asserts::verify_equal ("left inverse gives source-space identity", inverse * matrix, math::SquareMatrix<double, 2, Source> (math::identity));
		});
	});
});

} // namespace
} // namespace neutrino::test

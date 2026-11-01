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

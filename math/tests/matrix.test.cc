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
#include <array>
#include <cstddef>
#include <iterator>
#include <sstream>
#include <stdexcept>
#include <type_traits>


namespace neutrino::test {
namespace {

TestRegistration tests ([]{
	describe ("Neutrino", "math", "Matrix", []{
		it ("constructs a matrix from an exact iterator range in row-major order", []{
			constexpr std::array<float, 6> values { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f };
			constexpr math::Matrix<double, 3, 2> matrix (values.begin(), values.end());
			constexpr math::Matrix<double, 3, 2> expected { 1.0, 2.0, 3.0, 4.0, 5.0, 6.0 };

			static_assert (matrix == expected);
			test_asserts::verify ("all elements have the expected values and positions", matrix == expected);
		});

		it ("rejects empty, short and long iterator ranges", []{
			std::array<double, 5> const values { 1.0, 2.0, 3.0, 4.0, 5.0 };

			for (auto const end: { values.begin(), values.begin() + 3, values.end() })
			{
				test_asserts::verify_throws<std::length_error> ("rejects an incorrect element count", [&values, end] {
					math::Matrix<double, 2, 2> const matrix (values.begin(), end);
				});
			}
		});

		it ("constructs a matrix from single-pass input iterators", []{
			std::istringstream input ("1 2 3 4");
			math::Matrix<double, 2, 2> const matrix { std::istream_iterator<double> (input), std::istream_iterator<double>() };
			math::Matrix<double, 2, 2> const expected { 1.0, 2.0, 3.0, 4.0 };

			test_asserts::verify ("all input elements are preserved", matrix == expected);
		});

		it ("rejects single-pass input ranges with an incorrect element count", []{
			for (auto const text: { "", "1 2 3", "1 2 3 4 5" })
			{
				test_asserts::verify_throws<std::length_error> ("rejects an incorrect input element count", [text] {
					std::istringstream input (text);
					math::Matrix<double, 2, 2> const matrix { std::istream_iterator<double> (input), std::istream_iterator<double>() };
				});
			}
		});

		it ("normalizes a vector in place", []{
			math::Vector<double, 2> vector { 3.0, 4.0 };

			vector.normalize();

			test_asserts::verify_equal_with_epsilon ("normalized x", vector.x(), 0.6, 1e-12);
			test_asserts::verify_equal_with_epsilon ("normalized y", vector.y(), 0.8, 1e-12);
			test_asserts::verify_equal_with_epsilon ("normalized length", vector.norm(), 1.0, 1e-12);
		});

		it ("normalizes a vector with physical quantities in place", []{
			using namespace si::literals;

			math::Vector<si::Length, 2> vector { 3.0_m, 4.0_m };

			vector.normalize();

			test_asserts::verify_equal_with_epsilon ("normalized x", vector.x(), 0.6_m, 1e-12_m);
			test_asserts::verify_equal_with_epsilon ("normalized y", vector.y(), 0.8_m, 1e-12_m);
			test_asserts::verify_equal_with_epsilon ("normalized length", vector.norm(), 1.0_m, 1e-12_m);
		});

		it ("converts scalar type explicitly and implicitly", []{
			using FloatMatrix = math::Matrix<float, 2, 2>;
			using DoubleMatrix = math::Matrix<double, 2, 2>;

			static_assert (std::is_convertible_v<FloatMatrix, DoubleMatrix>);
			static_assert (!std::is_convertible_v<DoubleMatrix, FloatMatrix>);
			static_assert (!std::is_constructible_v<FloatMatrix, DoubleMatrix>);

			FloatMatrix const float_matrix {
				1.0f,	2.0f,
				3.0f,	4.0f,
			};

			DoubleMatrix const double_matrix = float_matrix;

			test_asserts::verify_equal ("float -> double converts [0, 0]", double_matrix[0, 0], 1.0);
			test_asserts::verify_equal ("float -> double converts [1, 0]", double_matrix[1, 0], 2.0);
			test_asserts::verify_equal ("float -> double converts [0, 1]", double_matrix[0, 1], 3.0);
			test_asserts::verify_equal ("float -> double converts [1, 1]", double_matrix[1, 1], 4.0);

			auto const narrowed_by_scalar = math::narrow_cast<float> (double_matrix);
			static_assert (std::is_same_v<decltype (narrowed_by_scalar), FloatMatrix const>);

			test_asserts::verify_equal ("narrow_cast<float> converts [0, 0]", narrowed_by_scalar[0, 0], 1.0f);
			test_asserts::verify_equal ("narrow_cast<float> converts [1, 0]", narrowed_by_scalar[1, 0], 2.0f);
			test_asserts::verify_equal ("narrow_cast<float> converts [0, 1]", narrowed_by_scalar[0, 1], 3.0f);
			test_asserts::verify_equal ("narrow_cast<float> converts [1, 1]", narrowed_by_scalar[1, 1], 4.0f);

			auto const narrowed_by_matrix = math::narrow_cast<FloatMatrix> (double_matrix);
			static_assert (std::is_same_v<decltype (narrowed_by_matrix), FloatMatrix const>);

			test_asserts::verify_equal ("narrow_cast<Matrix> converts [0, 0]", narrowed_by_matrix[0, 0], 1.0f);
			test_asserts::verify_equal ("narrow_cast<Matrix> converts [1, 0]", narrowed_by_matrix[1, 0], 2.0f);
			test_asserts::verify_equal ("narrow_cast<Matrix> converts [0, 1]", narrowed_by_matrix[0, 1], 3.0f);
			test_asserts::verify_equal ("narrow_cast<Matrix> converts [1, 1]", narrowed_by_matrix[1, 1], 4.0f);
		});
	});
});

} // namespace
} // namespace neutrino::test

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
#include <neutrino/test/auto_test.h>

// Standard:
#include <cstddef>
#include <type_traits>


namespace neutrino::test {
namespace {

AutoTest t1 ("math::Matrix scalar conversion", []{
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

} // namespace
} // namespace neutrino::test

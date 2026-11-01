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
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <iterator>
#include <sstream>
#include <stdexcept>
#include <type_traits>


namespace neutrino::test {
namespace {

TestRegistration tests ([]{
	describe ("Neutrino", "math", "Matrix", []{
		it ("inverts every 4x4 permutation matrix", []{
			using Matrix = math::SquareMatrix<double, 4>;
			std::array<std::size_t, 4> permutation { 0, 1, 2, 3 };
			Matrix const identity (math::identity);

			do {
				Matrix matrix;

				for (std::size_t row = 0; row < permutation.size(); ++row)
					matrix[permutation[row], row] = 1.0;

				auto const inverse = matrix.inverted();

				test_asserts::verify_equal ("inverse is the transpose", inverse, matrix.transposed());
				test_asserts::verify_equal ("right inverse gives identity", matrix * inverse, identity);
				test_asserts::verify_equal ("left inverse gives identity", inverse * matrix, identity);
			} while (std::next_permutation (permutation.begin(), permutation.end()));
		});

		it ("inverts a 4x4 matrix with a tiny diagonal element and a larger negative pivot", []{
			using Matrix = math::SquareMatrix<double, 4>;
			Matrix const matrix {
				1e-20,	1.0,	0.0,	0.0,
				-1.0,	1.0,	0.0,	0.0,
				0.0,	0.0,	2.0,	0.0,
				0.0,	0.0,	0.0,	3.0,
			};
			Matrix const expected {
				1.0,	-1.0,	0.0,	0.0,
				1.0,	1e-20,	0.0,	0.0,
				0.0,	0.0,	0.5,	0.0,
				0.0,	0.0,	0.0,	1.0 / 3.0,
			};
			auto const inverse = matrix.inverted();
			Matrix const identity (math::identity);

			test_asserts::verify_equal_with_epsilon ("inverse matches the expected values", inverse, expected, 1e-12);
			test_asserts::verify_equal_with_epsilon ("right inverse gives identity", matrix * inverse, identity, 1e-12);
			test_asserts::verify_equal_with_epsilon ("left inverse gives identity", inverse * matrix, identity, 1e-12);
		});

		it ("inverts a dense 5x5 matrix requiring row exchanges", []{
			using Matrix = math::SquareMatrix<double, 5>;
			Matrix const matrix {
				1.0,	6.0,	-1.0,	1.0,	1.0,
				1.0,	1.0,	7.0,	-1.0,	1.0,
				-1.0,	1.0,	1.0,	8.0,	1.0,
				1.0,	-1.0,	1.0,	1.0,	9.0,
				5.0,	1.0,	-1.0,	1.0,	1.0,
			};
			auto const inverse = matrix.inverted();
			Matrix const identity (math::identity);

			test_asserts::verify_equal_with_epsilon ("right inverse gives identity", matrix * inverse, identity, 1e-12);
			test_asserts::verify_equal_with_epsilon ("left inverse gives identity", inverse * matrix, identity, 1e-12);
		});

		it ("inverts a matrix with physical quantities and coordinate-system tags", []{
			using namespace si::literals;

			struct TargetSpace: math::CoordinateSystemBase { };
			struct SourceSpace: math::CoordinateSystemBase { };

			math::SquareMatrix<si::Length, 4, TargetSpace, SourceSpace> const matrix {
				0.0_m,	2.0_m,	0.0_m,	0.0_m,
				-3.0_m,	1.0_m,	0.0_m,	0.0_m,
				0.0_m,	0.0_m,	4.0_m,	0.0_m,
				0.0_m,	0.0_m,	0.0_m,	5.0_m,
			};
			auto const inverse = matrix.inverted();
			using ExpectedInverse = math::SquareMatrix<decltype (1.0 / 1_m), 4, SourceSpace, TargetSpace>;
			static_assert (std::is_same_v<decltype (inverse), ExpectedInverse const>);

			test_asserts::verify_equal_with_epsilon ("right inverse gives target-space identity",
													 matrix * inverse,
													 math::SquareMatrix<double, 4, TargetSpace> (math::identity),
													 1e-12);
			test_asserts::verify_equal_with_epsilon ("left inverse gives source-space identity",
													 inverse * matrix,
													 math::SquareMatrix<double, 4, SourceSpace> (math::identity),
													 1e-12);
		});

		it ("returns non-finite components for a singular 4x4 matrix", []{
			math::SquareMatrix<double, 4> const matrix {
				1.0,	0.0,	0.0,	0.0,
				0.0,	0.0,	0.0,	0.0,
				0.0,	0.0,	1.0,	0.0,
				0.0,	0.0,	0.0,	1.0,
			};
			auto const inverse = matrix.inverted();
			auto const is_non_finite = [](double value) { return !std::isfinite (value); };

			test_asserts::verify ("singular inverse contains infinities or NaNs",
								  std::any_of (inverse.components().begin(), inverse.components().end(), is_non_finite));
		});

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

		it ("converts nonzero real quaternions to identity rotation matrices", []{
			using Matrix = math::SquareMatrix<double, 3>;
			constexpr math::Quaternion<double> q { 2.0, 0.0, 0.0, 0.0 };
			constexpr Matrix matrix (q);
			constexpr Matrix identity (math::identity);
			static_assert (matrix == identity);
			math::Vector<double, 3> const vector { 1.0, -2.0, 3.0 };

			for (auto const w: { 2.0, -2.0, 0.5, -0.5 })
			{
				math::Quaternion<double> const real { w, 0.0, 0.0, 0.0 };
				Matrix const rotation (real);
				test_asserts::verify_equal ("real quaternion gives identity", rotation, identity);
				test_asserts::verify_equal ("matrix preserves the vector", rotation * vector, vector);
				test_asserts::verify_equal ("matrix agrees with quaternion action", rotation * vector, real * vector);
			}
		});

		it ("converts general quaternions to scale-invariant rotation matrices", []{
			using Matrix = math::SquareMatrix<double, 3>;
			using Vector = math::Vector<double, 3>;
			Matrix const identity (math::identity);

			for (auto const q: { math::Quaternion<double> { 1.0, 2.0, 3.0, 4.0 },
									math::Quaternion<double> { 0.0, 1.0, 2.0, 3.0 } })
			{
				Matrix const normalized_rotation (q.normalized());

				for (auto const factor: { 1.0, 2.0, -3.0, 0.25, 0.001 })
				{
					auto const scaled = factor * q;
					Matrix const rotation (scaled);
					test_asserts::verify_equal_with_epsilon ("rotation is independent of quaternion scale", rotation, normalized_rotation, 1e-12);
					test_asserts::verify_equal_with_epsilon ("rotation matrix is orthogonal", rotation.transposed() * rotation, identity, 1e-12);

					for (auto const& vector: { Vector { 1.0, 0.0, 0.0 }, Vector { 0.0, 1.0, 0.0 }, Vector { 0.0, 0.0, 1.0 }, Vector { -2.0, 3.0, 5.0 } })
						test_asserts::verify_equal_with_epsilon ("matrix agrees with quaternion action", rotation * vector, scaled * vector, 1e-12);
				}
			}
		});

		it ("converts flagged and general unit quaternions to the same rotation matrix", []{
			math::Quaternion<double, void, void, true> const rotation { 0.5, 0.5, 0.5, 0.5 };
			math::Quaternion<double> const general = rotation;
			math::SquareMatrix<double, 3> const expected {
				0.0, 0.0, 1.0,
				1.0, 0.0, 0.0,
				0.0, 1.0, 0.0,
			};

			test_asserts::verify_equal ("flagged quaternion gives expected rotation", math::SquareMatrix<double, 3> (rotation), expected);
			test_asserts::verify_equal ("general quaternion gives expected rotation", math::SquareMatrix<double, 3> (general), expected);
		});

		it ("converts float quaternions to rotation matrices", []{
			math::Quaternion<float> const q { 1.0f, 2.0f, 3.0f, 4.0f };
			math::SquareMatrix<float, 3> const matrix (q);
			math::Vector<float, 3> const vector { -2.0f, 3.0f, 5.0f };

			test_asserts::verify_equal_with_epsilon ("float matrix agrees with quaternion action", matrix * vector, q * vector, 1e-5f);
			math::Quaternion<float, void, void, true> const rotation (math::identity);
			test_asserts::verify_equal ("float rotation quaternion gives identity", math::SquareMatrix<float, 3> (rotation), math::SquareMatrix<float, 3> (math::identity));
		});

		it ("preserves coordinate tags and physical vector quantities when converting quaternions", []{
			using namespace si::literals;
			struct Target: math::CoordinateSystemBase { };
			struct Source: math::CoordinateSystemBase { };
			math::Quaternion<double, Target, Source> const q { 1.0, 2.0, 3.0, 4.0 };
			math::SquareMatrix<double, 3, Target, Source> const rotation (q);
			math::Vector<si::Length, 3, Source> const vector { -2.0_m, 3.0_m, 5.0_m };
			auto const rotated = rotation * vector;
			static_assert (std::is_same_v<decltype (rotated), math::Vector<si::Length, 3, Target> const>);

			test_asserts::verify_equal_with_epsilon ("tagged matrix agrees with quaternion action", rotated, q * vector, 1e-12_m);
			test_asserts::verify_equal_with_epsilon ("rotation preserves physical length", rotated.norm(), vector.norm(), 1e-12_m);
		});

		it ("produces non-finite entries for a zero quaternion with no defined rotation", []{
			math::Quaternion<double> const zero (math::zero);
			math::SquareMatrix<double, 3> const matrix (zero);
			auto const is_non_finite = [](double const value) { return !std::isfinite (value); };

			test_asserts::verify ("zero quaternion has no finite rotation matrix",
								  std::all_of (matrix.components().begin(), matrix.components().end(), is_non_finite));
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

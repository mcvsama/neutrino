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
#include <limits>
#include <sstream>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <utility>


namespace neutrino::test {
namespace {

template<class Left, class Right>
	concept MultiplyAssignable = requires (Left& lhs, Right const& rhs) { lhs *= rhs; };


template<class Matrix>
	concept HasTrace = requires (Matrix const& matrix) { math::trace (matrix); };


template<class Matrix, std::size_t I>
	concept HasTupleGet = requires (Matrix const& matrix) { matrix.template get<I>(); };


template<class Matrix>
	concept HasTupleSize = requires { std::tuple_size<Matrix>::value; };


TestRegistration tests ([]{
	describe ("Neutrino", "math", "Matrix", []{
		it ("puts a matrix at the origin and exactly against the destination edges", []{
			using Matrix = math::Matrix<double, 4, 3>;
			constexpr math::SquareMatrix<double, 2> source { 1.0, 2.0, 3.0, 4.0 };
			Matrix at_origin;
			at_origin.put (source, 0, 0);
			Matrix const expected_origin {
				1.0, 2.0, 0.0, 0.0,
				3.0, 4.0, 0.0, 0.0,
				0.0, 0.0, 0.0, 0.0,
			};
			test_asserts::verify_equal ("source is inserted at the origin", at_origin, expected_origin);

			constexpr auto at_edge = []{
				Matrix result;
				result.put (math::SquareMatrix<double, 2> { 1.0, 2.0, 3.0, 4.0 }, 2, 1);
				return result;
			}();
			constexpr Matrix expected_edge {
				0.0, 0.0, 0.0, 0.0,
				0.0, 0.0, 1.0, 2.0,
				0.0, 0.0, 3.0, 4.0,
			};
			static_assert (at_edge == expected_edge);
			test_asserts::verify_equal ("source fits exactly against the right and bottom edges", at_edge, expected_edge);

			Matrix full_size;
			full_size.put (expected_origin, 0, 0);
			test_asserts::verify_equal ("source may fill the entire destination", full_size, expected_origin);
		});

		it ("rejects invalid put positions before writing, including offsets that would overflow", []{
			math::Matrix<double, 4, 3> const original {
				1.0, 2.0, 3.0, 4.0,
				5.0, 6.0, 7.0, 8.0,
				9.0, 10.0, 11.0, 12.0,
			};
			math::SquareMatrix<double, 2> const source { 20.0, 30.0, 40.0, 50.0 };
			constexpr auto max = std::numeric_limits<std::size_t>::max();
			std::array<std::pair<std::size_t, std::size_t>, 11> const invalid_positions {{
				{ 3, 0 }, { 0, 2 }, { 4, 0 }, { 0, 3 }, { 5, 0 }, { 0, 4 },
				{ max, 0 }, { 0, max }, { max - 1, 0 }, { 0, max - 1 }, { max, max },
			}};

			for (auto const& [column, row]: invalid_positions)
			{
				auto matrix = original;
				test_asserts::verify_throws<std::out_of_range> ("source outside the destination is rejected", [&] {
					matrix.put (source, column, row);
				});
				test_asserts::verify_equal ("rejected insertion leaves every component unchanged", matrix, original);
			}
		});

		it ("allows empty source matrices at the destination boundaries", []{
			math::Matrix<double, 3, 2> matrix { 1.0, 2.0, 3.0, 4.0, 5.0, 6.0 };
			auto const original = matrix;
			matrix.put (math::Matrix<double, 0, 1>(), 3, 1);
			matrix.put (math::Matrix<double, 1, 0>(), 2, 2);
			matrix.put (math::Matrix<double, 0, 0>(), 3, 2);
			test_asserts::verify_equal ("empty insertions leave the destination unchanged", matrix, original);
		});

		it ("copies matrices when changing coordinate tags while preserving shape and scalar types", []{
			auto const verify_type = []<math::Scalar S> {
				struct OldTarget: math::CoordinateSystemBase { };
				struct OldSource: math::CoordinateSystemBase { };
				struct NewTarget: math::CoordinateSystemBase { };
				struct NewSource: math::CoordinateSystemBase { };
				using Original = math::Matrix<S, 3, 2, OldTarget, OldSource>;
				using Result = math::Matrix<S, 3, 2, NewTarget, NewSource>;

				constexpr Original original { S (1), S (2), S (3), S (4), S (5), S (6) };
				constexpr auto explicit_tags = math::coordinate_system_cast<NewTarget, NewSource, OldTarget, OldSource> (original);
				constexpr auto deduced_tags = math::coordinate_system_cast_any<NewTarget, NewSource> (original);

				static_assert (std::is_same_v<decltype (explicit_tags), Result const>);
				static_assert (std::is_same_v<decltype (deduced_tags), Result const>);
				static_assert (explicit_tags.components() == original.components());
				static_assert (deduced_tags == explicit_tags);

				auto source = original;
				auto copy = math::coordinate_system_cast<NewTarget, NewSource, OldTarget, OldSource> (source);
				auto deduced_copy = math::coordinate_system_cast_any<NewTarget, NewSource> (source);

				static_assert (std::is_same_v<decltype (copy), Result>);
				static_assert (std::is_same_v<decltype (deduced_copy), Result>);

				copy[0, 0] = S (9);
				deduced_copy[0, 0] = S (8);
				test_asserts::verify_equal ("changing either copy leaves the source unchanged", source, original);

				source[1, 0] = S (7);
				test_asserts::verify_equal ("explicit-tag copy has independent storage", copy[1, 0], S (2));
				test_asserts::verify_equal ("deduced-tag copy has independent storage", deduced_copy[1, 0], S (2));
			};

			verify_type.operator()<float>();
			verify_type.operator()<double>();
			verify_type.operator()<si::Quantity<si::Meter, float>>();
			verify_type.operator()<si::Length>();
		});

		it ("keeps coordinate-cast results alive after temporary vectors and scalar matrices expire", []{
			struct Space: math::CoordinateSystemBase { };
			auto const& vector = math::coordinate_system_cast_any<Space, void> (math::Vector<float, 3> { 1.0f, 2.0f, 3.0f });
			auto const& scalar = math::coordinate_system_cast<void, void, void, void> (math::SquareMatrix<double, 1> { 42.0 });

			test_asserts::verify_equal ("temporary vector's first component is copied", vector[0], 1.0f);
			test_asserts::verify_equal ("temporary vector's last component is copied", vector[2], 3.0f);
			test_asserts::verify_equal ("temporary scalar matrix is copied", scalar.scalar(), 42.0);

			constexpr math::Matrix<float, 3, 1> row { 1.0f, 2.0f, 3.0f };
			constexpr auto copied_row = math::coordinate_system_cast_any<Space, void> (row);

			static_assert (std::is_same_v<decltype (copied_row), math::Matrix<float, 3, 1, Space, void> const>);
			static_assert (copied_row.components() == row.components());
		});

		it ("decomposes scalar matrices without ambiguous tuple specializations", []{
			auto const verify_type = []<std::floating_point Value> {
				struct Target: math::CoordinateSystemBase { };
				struct Source: math::CoordinateSystemBase { };
				using Matrix = math::Matrix<Value, 1, 1, Target, Source>;

				static_assert (std::tuple_size_v<Matrix> == 1);
				static_assert (std::tuple_size_v<Matrix const> == 1);
				static_assert (std::is_same_v<std::tuple_element_t<0, Matrix>, Value>);
				static_assert (std::is_same_v<std::tuple_element_t<0, Matrix const>, Value const>);
				static_assert ([]{
					constexpr Matrix matrix { Value (2) };
					auto const [value] = matrix;
					return value == Value (2);
				}());

				Matrix matrix { Value (2) };
				auto [value] = matrix;
				auto const& [const_value] = std::as_const (matrix);
				auto [temporary_value] = Matrix { Value (3) };

				static_assert (std::is_same_v<decltype (value), Value>);
				static_assert (std::is_same_v<decltype (const_value), Value const>);

				test_asserts::verify_equal ("scalar matrix exposes its single value", value, Value (2));
				test_asserts::verify_equal ("const scalar matrix exposes its single value", const_value, Value (2));
				test_asserts::verify_equal ("temporary scalar matrix can be decomposed", temporary_value, Value (3));
			};

			verify_type.operator()<float>();
			verify_type.operator()<double>();
			verify_type.operator()<long double>();
		});

		it ("decomposes row and column vectors while preserving physical units", []{
			using namespace si::literals;
			auto const verify_shape = []<std::size_t Columns, std::size_t Rows> {
				using Matrix = math::Matrix<si::Length, Columns, Rows>;

				static_assert (std::tuple_size_v<Matrix> == 3);
				static_assert (std::is_same_v<std::tuple_element_t<2, Matrix>, si::Length>);
				static_assert ([]{
					constexpr Matrix matrix { 1_m, -2_m, 3_m };
					auto const [x, y, z] = matrix;
					return x == 1_m && y == -2_m && z == 3_m;
				}());

				Matrix matrix { 1_m, -2_m, 3_m };
				auto [x, y, z] = matrix;
				auto const& [cx, cy, cz] = std::as_const (matrix);

				static_assert (std::is_same_v<decltype (x), si::Length>);
				static_assert (std::is_same_v<decltype (cx), si::Length const>);

				test_asserts::verify_equal ("first component retains length units", x, 1_m);
				test_asserts::verify_equal ("second component retains length units", y, -2_m);
				test_asserts::verify_equal ("third component retains length units", z, 3_m);
				test_asserts::verify ("const decomposition preserves component order", cx == x && cy == y && cz == z);
			};

			verify_shape.operator()<1, 3>();
			verify_shape.operator()<3, 1>();

			math::SquareMatrix<si::Length, 1> const scalar { 4_m };
			auto const [value] = scalar;
			static_assert (std::is_same_v<decltype (value), si::Length const>);
			test_asserts::verify_equal ("scalar decomposition retains length units", value, 4_m);
		});

		it ("restricts tuple access to valid vector and scalar indices", []{
			using Scalar = math::SquareMatrix<double, 1>;
			using Column = math::Matrix<double, 1, 3>;
			using Row = math::Matrix<double, 3, 1>;
			using Matrix = math::SquareMatrix<double, 2>;

			static_assert (HasTupleGet<Scalar, 0>);
			static_assert (!HasTupleGet<Scalar, 1>);
			static_assert (HasTupleGet<Column, 2>);
			static_assert (!HasTupleGet<Column, 3>);
			static_assert (HasTupleGet<Row, 2>);
			static_assert (!HasTupleGet<Row, 3>);
			static_assert (!HasTupleSize<Matrix>);
			static_assert (!HasTupleGet<Matrix, 0>);
		});

		it ("negates floating-point matrices without changing scalar types or coordinate tags", []{
			auto const verify_type = []<std::floating_point Value> {
				struct Target: math::CoordinateSystemBase { };
				struct Source: math::CoordinateSystemBase { };
				using Matrix = math::Matrix<Value, 3, 2, Target, Source>;

				constexpr Matrix matrix { Value (1), Value (-2), Value (0), Value (-4), Value (5), Value (-6) };
				constexpr Matrix expected { Value (-1), Value (2), Value (0), Value (4), Value (-5), Value (6) };
				constexpr auto negated = -matrix;

				static_assert (std::is_same_v<decltype (negated), Matrix const>);
				static_assert (negated == expected);
				static_assert (matrix.operator-() == expected);
				static_assert (-negated == matrix);

				test_asserts::verify_equal ("every component is negated", negated, expected);
			};

			verify_type.operator()<float>();
			verify_type.operator()<double>();
			verify_type.operator()<long double>();
		});

		it ("preserves physical units when negating a matrix", []{
			using namespace si::literals;
			using Matrix = math::Matrix<si::Length, 3, 2>;

			constexpr Matrix matrix { 1_m, -2_m, 0_m, -4_m, 5_m, -6_m };
			constexpr Matrix expected { -1_m, 2_m, 0_m, 4_m, -5_m, 6_m };
			constexpr auto negated = -matrix;

			static_assert (std::is_same_v<decltype (negated), Matrix const>);
			static_assert (negated == expected);
			static_assert (matrix.operator-() == expected);

			test_asserts::verify_equal ("negated components retain length units", negated, expected);
		});

		it ("negates float vectors and scalar matrices", []{
			constexpr math::Vector<float, 3> vector { 1.0f, -2.0f, 3.0f };
			constexpr math::Vector<float, 3> expected { -1.0f, 2.0f, -3.0f };
			constexpr auto negated = -vector;

			static_assert (std::is_same_v<decltype (negated), math::Vector<float, 3> const>);
			static_assert (negated == expected);

			constexpr math::SquareMatrix<float, 1> scalar { 2.0f };
			constexpr auto negated_scalar = -scalar;

			static_assert (std::is_same_v<decltype (negated_scalar), math::SquareMatrix<float, 1> const>);
			static_assert (negated_scalar.components()[0] == -2.0f);

			test_asserts::verify_equal ("vector components are negated", negated, expected);
			test_asserts::verify_equal ("scalar component is negated", negated_scalar[0, 0], -2.0f);
		});

		it ("multiplies rectangular floating-point matrices by a scalar in place", []{
			auto const verify_type = []<std::floating_point Value> {
				using Matrix = math::Matrix<Value, 3, 2>;
				Matrix matrix { Value (1), Value (2), Value (3), Value (4), Value (5), Value (6) };
				Matrix const expected { Value (2), Value (4), Value (6), Value (8), Value (10), Value (12) };
				static_assert (noexcept (matrix *= Value (2)));
				auto& result = matrix *= Value (2);

				test_asserts::verify_equal ("all components are scaled", matrix, expected);
				test_asserts::verify ("scalar assignment returns this matrix", &result == &matrix);
			};

			verify_type.operator()<float>();
			verify_type.operator()<double>();
			verify_type.operator()<long double>();
		});

		it ("multiplies square matrices in place using matrix multiplication", []{
			math::SquareMatrix<double, 2> matrix { 1.0, 2.0, 3.0, 4.0 };
			math::SquareMatrix<double, 2> const other { 5.0, 6.0, 7.0, 8.0 };
			math::SquareMatrix<double, 2> const expected { 19.0, 22.0, 43.0, 50.0 };
			auto& result = matrix *= other;

			test_asserts::verify_equal ("result is the matrix product", matrix, expected);
			test_asserts::verify ("matrix assignment returns this matrix", &result == &matrix);
		});

		it ("handles self-multiplication without overwriting input components", []{
			math::SquareMatrix<double, 2> matrix { 1.0, 2.0, 3.0, 4.0 };
			math::SquareMatrix<double, 2> const expected { 7.0, 10.0, 15.0, 22.0 };
			matrix *= matrix;

			test_asserts::verify_equal ("self-multiplication computes the matrix square", matrix, expected);
		});

		it ("supports matrix multiplication with widening scalar conversion", []{
			using DoubleMatrix = math::SquareMatrix<double, 2>;
			using FloatMatrix = math::SquareMatrix<float, 2>;
			static_assert (MultiplyAssignable<DoubleMatrix, FloatMatrix>);
			static_assert (!MultiplyAssignable<FloatMatrix, DoubleMatrix>);
			DoubleMatrix matrix { 1.0, 2.0, 3.0, 4.0 };
			FloatMatrix const other { 5.0f, 6.0f, 7.0f, 8.0f };
			DoubleMatrix const expected { 19.0, 22.0, 43.0, 50.0 };
			matrix *= other;

			test_asserts::verify_equal ("float matrix can multiply a double matrix in place", matrix, expected);
		});

		it ("supports in-place multiplication that preserves physical units", []{
			using namespace si::literals;
			using Matrix = math::SquareMatrix<si::Length, 2>;
			static_assert (MultiplyAssignable<Matrix, double>);
			static_assert (MultiplyAssignable<Matrix, math::SquareMatrix<double, 2>>);
			static_assert (!MultiplyAssignable<Matrix, si::Length>);
			static_assert (!MultiplyAssignable<Matrix, Matrix>);
			static_assert (!MultiplyAssignable<Matrix, math::SquareMatrix<si::Time, 2>>);
			static_assert (!MultiplyAssignable<math::SquareMatrix<double, 2>, Matrix>);
			Matrix matrix { 1_m, 2_m, 3_m, 4_m };
			matrix *= 2.0;
			test_asserts::verify_equal ("scalar multiplication preserves length", matrix, Matrix { 2_m, 4_m, 6_m, 8_m });
			matrix *= math::SquareMatrix<double, 2> { 5.0, 6.0, 7.0, 8.0 };
			test_asserts::verify_equal ("matrix multiplication preserves length", matrix, Matrix { 38_m, 44_m, 86_m, 100_m });
		});

		it ("rejects incompatible matrix shapes, coordinate systems and non-const scalar conversions", []{
			using Rectangular = math::Matrix<double, 3, 2>;
			using Square = math::SquareMatrix<double, 2>;
			static_assert (MultiplyAssignable<Rectangular, double>);
			static_assert (!MultiplyAssignable<Rectangular, Rectangular>);
			static_assert (!MultiplyAssignable<Square, math::SquareMatrix<double, 3>>);
			struct Target: math::CoordinateSystemBase { };
			struct Source: math::CoordinateSystemBase { };
			using Endomorphism = math::SquareMatrix<double, 2, Target>;
			using DifferentSpaces = math::SquareMatrix<double, 2, Target, Source>;
			static_assert (MultiplyAssignable<Endomorphism, Endomorphism>);
			static_assert (!MultiplyAssignable<DifferentSpaces, DifferentSpaces>);

			struct MutableFactor
			{
				operator double()
					{ return 2.0; }
			};
			static_assert (!MultiplyAssignable<Square, MutableFactor>);
		});

		it ("propagates exceptions from scalar conversion instead of terminating", []{
			struct ThrowingFactor
			{
				operator double() const
					{ throw std::runtime_error ("factor conversion failed"); }
			};
			using Matrix = math::SquareMatrix<double, 2>;
			static_assert (MultiplyAssignable<Matrix, ThrowingFactor>);
			static_assert (!noexcept (std::declval<Matrix&>() *= std::declval<ThrowingFactor const&>()));
			Matrix matrix { 1.0, 2.0, 3.0, 4.0 };

			test_asserts::verify_throws<std::runtime_error> ("conversion exception reaches the caller", [&matrix] {
				matrix *= ThrowingFactor();
			});
		});

		it ("transforms every component of a rectangular matrix in storage order", []{
			math::Matrix<double, 3, 2> const matrix { 1.0, 2.0, 3.0, 4.0, 5.0, 6.0 };
			math::Matrix<double, 3, 2> const expected { 3.0, 5.0, 7.0, 9.0, 11.0, 13.0 };
			std::array<double, 6> visited {};
			std::size_t calls = 0;
			auto const transformed = matrix.transformed ([&visited, &calls](double const& value) {
				visited.at (calls++) = value;
				return 2.0 * value + 1.0;
			});

			test_asserts::verify_equal ("all matrix components are transformed", transformed, expected);
			test_asserts::verify_equal ("each component is visited once", calls, matrix.components().size());
			test_asserts::verify ("transform sees the original components in row-major order", visited == matrix.components());
		});

		it ("allows transformed matrices to change scalar type while preserving shape and coordinate tags", []{
			struct Target: math::CoordinateSystemBase { };
			struct Source: math::CoordinateSystemBase { };
			using FloatMatrix = math::Matrix<float, 3, 2, Target, Source>;
			constexpr math::Matrix<double, 3, 2, Target, Source> matrix { 1.0, 2.0, 3.0, 4.0, 5.0, 6.0 };
			constexpr auto transformed = matrix.transformed ([](double const& value) { return static_cast<float> (value / 2.0); });
			constexpr FloatMatrix expected { 0.5f, 1.0f, 1.5f, 2.0f, 2.5f, 3.0f };
			static_assert (std::is_same_v<decltype (transformed), FloatMatrix const>);
			static_assert (transformed == expected);

			test_asserts::verify_equal ("transformed values have the requested scalar type", transformed, expected);
		});

		it ("transforms matrices of physical quantities into a different physical dimension", []{
			using namespace si::literals;
			math::SquareMatrix<si::Length, 2> const matrix { 1_m, -2_m, 3_m, -4_m };
			auto const transformed = matrix.transformed ([](si::Length const& value) { return value * value; });
			static_assert (std::is_same_v<decltype (transformed), math::SquareMatrix<si::Area, 2> const>);
			math::SquareMatrix<si::Area, 2> const expected { 1_m * 1_m, 2_m * 2_m, 3_m * 3_m, 4_m * 4_m };

			test_asserts::verify_equal ("squared components retain their resulting physical dimension", transformed, expected);
		});

		it ("copies reference-valued transform results and calls the transform as an lvalue", []{
			struct BorrowComponent
			{
				double const&
				operator() (double const& value) &
					{ return value; }

				double const&
				operator() (double const&) && = delete;
			};
			math::SquareMatrix<double, 2> const matrix { 1.0, 2.0, 3.0, 4.0 };
			auto transformed = matrix.transformed (BorrowComponent());
			static_assert (std::is_same_v<decltype (transformed), math::SquareMatrix<double, 2>>);

			test_asserts::verify_equal ("returned references are copied as values", transformed, matrix);
			transformed[0, 0] = 9.0;
			test_asserts::verify_equal ("transformed storage is independent of the source", matrix[0, 0], 1.0);
		});

		it ("transforms vectors and scalar matrices", []{
			constexpr math::Vector<float, 3> vector { 1.0f, -2.0f, 3.0f };
			constexpr auto transformed_vector = vector.transformed ([](float const value) { return -value; });
			constexpr math::Vector<float, 3> expected { -1.0f, 2.0f, -3.0f };
			static_assert (transformed_vector == expected);
			constexpr math::SquareMatrix<double, 1> scalar { 2.0 };
			constexpr auto transformed_scalar = scalar.transformed ([](double const value) { return value * value; });
			static_assert (transformed_scalar.components()[0] == 4.0);

			test_asserts::verify_equal ("vector components are transformed", transformed_vector, expected);
			test_asserts::verify_equal ("scalar matrix component is transformed", transformed_scalar[0, 0], 4.0);
		});

		it ("computes the trace from diagonal elements and preserves the scalar type", []{
			auto const verify_type = []<std::floating_point Value> {
				constexpr math::SquareMatrix<Value, 3> matrix {
					Value (1), Value (100), Value (200),
					Value (300), Value (-4), Value (400),
					Value (500), Value (600), Value (6),
				};
				static_assert (std::is_same_v<decltype (math::trace (matrix)), Value>);
				static_assert (math::trace (matrix) == Value (3));
				test_asserts::verify_equal ("trace ignores off-diagonal elements", math::trace (matrix), Value (3));
			};

			verify_type.operator()<float>();
			verify_type.operator()<double>();
			verify_type.operator()<long double>();
			static_assert (HasTrace<math::SquareMatrix<double, 3>>);
			static_assert (!HasTrace<math::Matrix<double, 3, 2>>);
		});

		it ("computes traces of scalar, identity and zero matrices", []{
			constexpr math::SquareMatrix<double, 1> scalar { -2.5 };
			constexpr math::SquareMatrix<double, 4> identity (math::identity);
			constexpr math::SquareMatrix<double, 3> zero (math::zero);
			static_assert (math::trace (scalar) == -2.5);
			static_assert (math::trace (identity) == 4.0);
			static_assert (math::trace (zero) == 0.0);

			test_asserts::verify_equal ("scalar trace is the single component", math::trace (scalar), -2.5);
			test_asserts::verify_equal ("identity trace equals its dimension", math::trace (identity), 4.0);
			test_asserts::verify_equal ("zero matrix trace is zero", math::trace (zero), 0.0);
		});

		it ("computes the trace of a matrix with physical units and coordinate tags", []{
			using namespace si::literals;
			struct Space: math::CoordinateSystemBase { };
			math::SquareMatrix<si::Length, 3, Space> const matrix {
				1_m, 100_m, 200_m,
				300_m, 3_m, 400_m,
				500_m, 600_m, -5_m,
			};
			static_assert (std::is_same_v<decltype (math::trace (matrix)), si::Length>);

			test_asserts::verify_equal ("trace retains physical units", math::trace (matrix), -1_m);
		});

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

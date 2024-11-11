/* vim:ts=4
 *
 * Copyleft 2012…2017  Michał Gawron
 * Marduk Unix Labs, http://mulabs.org/
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Visit http://www.gnu.org/licenses/gpl-3.0.html for more information on licensing.
 */

#ifndef NEUTRINO__MATH__MATRIX_OPERATIONS_H__INCLUDED
#define NEUTRINO__MATH__MATRIX_OPERATIONS_H__INCLUDED

// Local:
#include "traits.h"

// Neutrino:
#include <neutrino/si/si.h>
#include <neutrino/types.h>

// Standard:
#include <algorithm>
#include <utility>
#include <type_traits>


namespace neutrino::math {

template<class Test,
		 template<class, std::size_t, std::size_t, class, class> class Ref>
	struct is_matrix: public std::false_type
	{ };


template<template<class, std::size_t, std::size_t, class, class> class Ref,
		 class Scalar, std::size_t Columns, std::size_t Rows, class TargetSpace, class SourceSpace>
	struct is_matrix<Ref<Scalar, Columns, Rows, TargetSpace, SourceSpace>, Ref>: public std::true_type
	{ };


template<class ScalarA, class ScalarB, std::size_t ARows, std::size_t Common, std::size_t BColumns, class TargetSpace, class IntermediateSpace, class SourceSpace>
	[[nodiscard]]
	constexpr auto
	operator* (Matrix<ScalarA, Common, ARows, TargetSpace, IntermediateSpace> const& a,
			   Matrix<ScalarB, BColumns, Common, IntermediateSpace, SourceSpace> const& b)
	{
		using ResultScalar = decltype (ScalarA{} * ScalarB{});

		auto result = Matrix<ResultScalar, BColumns, ARows, TargetSpace, SourceSpace> (uninitialized);

		for (std::size_t r = 0; r < ARows; ++r)
		{
			for (std::size_t c = 0; c < BColumns; ++c)
			{
				ResultScalar scalar{};

				for (std::size_t i = 0; i < Common; ++i)
					scalar += a[i, r] * b[c, i];

				result[c, r] = scalar;
			}
		}

		return result;
	}


template<class ScalarA, class ScalarB, std::size_t Columns, std::size_t Rows, class TargetSpace, class SourceSpace>
	[[nodiscard]]
	constexpr auto
	operator* (Matrix<ScalarA, Columns, Rows, TargetSpace, SourceSpace> matrix,
			   ScalarB const& scalar)
	{
		auto result = Matrix<decltype (ScalarA{} * ScalarB{}), Columns, Rows, TargetSpace, SourceSpace> (uninitialized);

		for (std::size_t i = 0; i < Columns * Rows; ++i)
			result.array().data()[i] = matrix.array().data()[i] * scalar;

		return result;
	}


template<class ScalarA, class ScalarB, std::size_t Columns, std::size_t Rows, class TargetSpace, class SourceSpace>
	[[nodiscard]]
	constexpr auto
	operator* (ScalarA const& scalar,
			   Matrix<ScalarB, Columns, Rows, TargetSpace, SourceSpace> const& matrix)
	{
		return matrix * scalar;
	}


template<class ScalarA, class ScalarB, std::size_t Columns, std::size_t Rows, class TargetSpace, class SourceSpace>
	[[nodiscard]]
	constexpr auto
	operator/ (Matrix<ScalarA, Columns, Rows, TargetSpace, SourceSpace> matrix,
			   ScalarB const& scalar)
	{
		auto result = Matrix<decltype (ScalarA{} / ScalarB{}), Columns, Rows, TargetSpace, SourceSpace> (uninitialized);
		auto const* src_data = matrix.array().data();
		auto* dst_data = result.array().data();

		for (std::size_t i = 0; i < Columns * Rows; ++i)
			dst_data[i] = src_data[i] / scalar;

		return result;
	}


template<class Scalar, std::size_t Columns, std::size_t Rows, class TargetSpace, class SourceSpace>
	[[nodiscard]]
	constexpr Matrix<Scalar, Columns, Rows, TargetSpace, SourceSpace>
	operator+ (Matrix<Scalar, Columns, Rows, TargetSpace, SourceSpace> m) noexcept
	{
		return m;
	}


template<class Scalar, std::size_t Columns, std::size_t Rows, class TargetSpace, class SourceSpace>
	[[nodiscard]]
	constexpr auto
	operator+ (Matrix<Scalar, Columns, Rows, TargetSpace, SourceSpace> a,
			   Matrix<Scalar, Columns, Rows, TargetSpace, SourceSpace> const& b)
		noexcept (noexcept (a += b))
	{
		return a += b;
	}


template<class Scalar, std::size_t Columns, std::size_t Rows, class TargetSpace, class SourceSpace>
	[[nodiscard]]
	constexpr auto
	operator- (Matrix<Scalar, Columns, Rows, TargetSpace, SourceSpace> m)
		noexcept (noexcept (m[0, 0] = -m[0, 0]))
	{
		for (std::size_t r = 0; r < Rows; ++r)
			for (std::size_t c = 0; c < Columns; ++c)
				m[c, r] = -m[c, r];

		return m;
	}


template<class Scalar, std::size_t Columns, std::size_t Rows, class TargetSpace, class SourceSpace>
	[[nodiscard]]
	constexpr auto
	operator- (Matrix<Scalar, Columns, Rows, TargetSpace, SourceSpace> a,
			   Matrix<Scalar, Columns, Rows, TargetSpace, SourceSpace> const& b)
		noexcept (noexcept (a -= b))
	{
		return a -= b;
	}


/**
 * Return dot product of two vectors.
 */
template<class ScalarA, class ScalarB, std::size_t Size, class TargetSpace, class SourceSpace>
	[[nodiscard]]
	constexpr decltype (ScalarA{} * ScalarB{})
	dot_product (Vector<ScalarA, Size, TargetSpace, SourceSpace> const& a,
				 Vector<ScalarB, Size, TargetSpace, SourceSpace> const& b)
		noexcept (noexcept ((~a * b).scalar()))
	{
		return (~a * b).scalar();
	}


/**
 * Return outer product of two vectors.
 */
template<class ScalarA, class ScalarB, std::size_t Size, class TargetSpace, class SourceSpace>
	[[nodiscard]]
	constexpr auto
	outer_product (Vector<ScalarA, Size, TargetSpace, SourceSpace> const& a,
				   Vector<ScalarB, Size, TargetSpace, SourceSpace> const& b)
		noexcept (noexcept (a * ~b))
	{
		return a * ~b;
	}


/**
 * Return cross product of two vectors.
 */
template<class ScalarA, class ScalarB, std::size_t Size, class TargetSpace, class SourceSpace>
	[[nodiscard]]
	constexpr Vector<decltype (ScalarA{} * ScalarB{}), Size, TargetSpace, SourceSpace>
	cross_product (Vector<ScalarA, Size, TargetSpace, SourceSpace> const& a,
				   Vector<ScalarB, Size, TargetSpace, SourceSpace> const& b)
		noexcept (noexcept (a[0] * b[0] - a[0] * b[0]))
	{
		return {
			a[1] * b[2] - a[2] * b[1],
			a[2] * b[0] - a[0] * b[2],
			a[0] * b[1] - a[1] * b[0],
		};
	}


/**
 * Return norm for a vector (aka absolute value aka length).
 */
template<class Scalar, std::size_t Size, class TargetSpace, class SourceSpace>
	[[nodiscard]]
	constexpr auto
	abs (Vector<Scalar, Size, TargetSpace, SourceSpace> const& v)
	{
		using std::sqrt;

		decltype (Scalar() * Scalar()) sum_of_squares (0);

		for (std::size_t i = 0; i < Size; ++i)
			sum_of_squares += v[i] * v[i];

		return sqrt (sum_of_squares);
	}


/**
 * Return Euclidean norm for a matrix.
 */
template<class Scalar, std::size_t Columns, std::size_t Rows, class TargetSpace, class SourceSpace>
	[[nodiscard]]
	constexpr auto
	euclidean_norm (Matrix<Scalar, Columns, Rows, TargetSpace, SourceSpace> const& m)
	{
		using SquareScalar = decltype (Scalar{} * Scalar{});
		SquareScalar norm (0);

		for (std::size_t i = 0; i < m.array().size(); ++i)
		{
			auto value = m.array()[i];
			norm += value * value;
		}

		return Scalar (std::sqrt (norm / SquareScalar (1)));
	}


/**
 * Return inverted matrix.
 */
template<class Scalar, std::size_t Columns, std::size_t Rows, class TargetSpace, class SourceSpace>
	[[nodiscard]]
	constexpr auto
	inv (Matrix<Scalar, Columns, Rows, TargetSpace, SourceSpace> const& matrix)
	{
		return matrix.inverted();
	}


/**
 * Return trace of the matrix.
 */
template<class Scalar, std::size_t Size, class TargetSpace, class SourceSpace>
	[[nodiscard]]
	constexpr auto
	trace (SquareMatrix<Scalar, Size, TargetSpace, SourceSpace> const& matrix)
	{
		Scalar result { 0 };

		for (std::size_t i = 0; i < Size; ++i)
			result += matrix (i, i);

		return result;
	}


/**
 * Traits for Matrix<>
 */
template<class Scalar, std::size_t Columns, std::size_t Rows, class TargetSpace, class SourceSpace>
	struct Traits<Matrix<Scalar, Columns, Rows, TargetSpace, SourceSpace>>
	{
		typedef Matrix<Scalar, Columns, Rows, TargetSpace, SourceSpace> Value;

		static Value
		zero();

		static Value
		unit();

		static Value
		inverted (Value const&);
	};


template<class Scalar, std::size_t Columns, std::size_t Rows, class TargetSpace, class SourceSpace>
	[[nodiscard]]
	inline auto
	Traits<Matrix<Scalar, Columns, Rows, TargetSpace, SourceSpace>>::zero() -> Value
	{
		return math::zero;
	}


template<class Scalar, std::size_t Columns, std::size_t Rows, class TargetSpace, class SourceSpace>
	[[nodiscard]]
	inline auto
	Traits<Matrix<Scalar, Columns, Rows, TargetSpace, SourceSpace>>::unit() -> Value
	{
		return math::unit;
	}


template<class Scalar, std::size_t Columns, std::size_t Rows, class TargetSpace, class SourceSpace>
	[[nodiscard]]
	inline auto
	Traits<Matrix<Scalar, Columns, Rows, TargetSpace, SourceSpace>>::inverted (Value const& v) -> Value
	{
		return v.inverted();
	}

} // namespace neutrino::math

#endif


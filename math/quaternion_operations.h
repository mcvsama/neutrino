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

#ifndef NEUTRINO__MATH__QUATERNION_OPERATIONS_H__INCLUDED
#define NEUTRINO__MATH__QUATERNION_OPERATIONS_H__INCLUDED

// Local:
#include "matrix.h"

// Standard:
#include <cmath>


namespace neutrino::math {

template<Scalar S, CoordinateSystem TargetSpace, CoordinateSystem SourceSpace, bool IsRotationQuaternion>
	[[nodiscard]]
	constexpr Quaternion<S, TargetSpace, SourceSpace, IsRotationQuaternion>
	operator+ (Quaternion<S, TargetSpace, SourceSpace, IsRotationQuaternion> const& q) noexcept
	{
		return q;
	}


template<
	Scalar SA,
	Scalar SB,
	CoordinateSystem TargetSpace,
	CoordinateSystem SourceSpace,
	bool IsRotationQuaternionA,
	bool IsRotationQuaternionB
>
	[[nodiscard]]
	constexpr auto
	operator+ (Quaternion<SA, TargetSpace, SourceSpace, IsRotationQuaternionA> const& a,
			   Quaternion<SB, TargetSpace, SourceSpace, IsRotationQuaternionB> const& b)
		noexcept (noexcept (a.real() + b.real(), a.imag() + b.imag()))
	{
		using ResultScalar = decltype (std::declval<SA>() + std::declval<SB>());
		return Quaternion<ResultScalar, TargetSpace, SourceSpace, IsRotationQuaternionA && IsRotationQuaternionB> (a.real() + b.real(), a.imag() + b.imag());
	}


template<Scalar S, CoordinateSystem TargetSpace, CoordinateSystem SourceSpace, bool IsRotationQuaternion>
	[[nodiscard]]
	constexpr Quaternion<S, TargetSpace, SourceSpace, IsRotationQuaternion>
	operator- (Quaternion<S, TargetSpace, SourceSpace, IsRotationQuaternion> const& q)
		noexcept (noexcept (-std::declval<S>()))
	{
		return Quaternion<S, TargetSpace, SourceSpace, IsRotationQuaternion> (-q.w(), -q.x(), -q.y(), -q.z());
	}


template<
	Scalar SA,
	Scalar SB,
	CoordinateSystem TargetSpace,
	CoordinateSystem SourceSpace,
	bool IsRotationQuaternionA,
	bool IsRotationQuaternionB
>
	[[nodiscard]]
	constexpr auto
	operator- (Quaternion<SA, TargetSpace, SourceSpace, IsRotationQuaternionA> const& a,
			   Quaternion<SB, TargetSpace, SourceSpace, IsRotationQuaternionB> const& b)
		noexcept (noexcept (a.real() - b.real(), a.imag() - b.imag()))
	{
		using ResultScalar = decltype (std::declval<SA>() - std::declval<SB>());
		return Quaternion<ResultScalar, TargetSpace, SourceSpace, IsRotationQuaternionA && IsRotationQuaternionB> (a.real() - b.real(), a.imag() - b.imag());
	}


template<Scalar S, CoordinateSystem TargetSpace, CoordinateSystem SourceSpace, bool IsRotationQuaternion>
	[[nodiscard]]
	constexpr auto
	operator* (Quaternion<S, TargetSpace, SourceSpace, IsRotationQuaternion> const& q,
			   S const scalar)
		noexcept (noexcept (q.real() * scalar) && noexcept (q.imag() * scalar))
	{
		return Quaternion<S, TargetSpace, SourceSpace, IsRotationQuaternion> (q.real() * scalar, q.imag() * scalar);
	}


template<Scalar S, CoordinateSystem TargetSpace, CoordinateSystem SourceSpace, bool IsRotationQuaternion>
	[[nodiscard]]
	constexpr auto
	operator* (S const scalar,
			   Quaternion<S, TargetSpace, SourceSpace, IsRotationQuaternion> const& q)
		noexcept (noexcept (q * scalar))
	{
		return q * scalar;
	}


template<
	Scalar SA,
	Scalar SB,
	CoordinateSystem TargetSpace,
	CoordinateSystem IntermediateSpace,
	CoordinateSystem SourceSpace,
	bool IsRotationQuaternionA,
	bool IsRotationQuaternionB
>
	[[nodiscard]]
	constexpr auto
	operator* (Quaternion<SA, TargetSpace, IntermediateSpace, IsRotationQuaternionA> const& a,
			   Quaternion<SB, IntermediateSpace, SourceSpace, IsRotationQuaternionB> const& b)
		noexcept (noexcept (a.w() * b.w() - a.x() * b.x() + a.y() * b.y() - a.z() * b.z()))
	{
		auto const w1 = a.w();	auto const w2 = b.w();
		auto const x1 = a.x();	auto const x2 = b.x();
		auto const y1 = a.y();	auto const y2 = b.y();
		auto const z1 = a.z();	auto const z2 = b.z();

		auto const w = w1 * w2 - x1 * x2 - y1 * y2 - z1 * z2;
		auto const x = w1 * x2 + x1 * w2 + y1 * z2 - z1 * y2;
		auto const y = w1 * y2 - x1 * z2 + y1 * w2 + z1 * x2;
		auto const z = w1 * z2 + x1 * y2 - y1 * x2 + z1 * w2;

		using ResultScalar = decltype (std::declval<SA>() * std::declval<SB>());
		return Quaternion<ResultScalar, TargetSpace, SourceSpace, IsRotationQuaternionA && IsRotationQuaternionB> (w, x, y, z);
	}


template<Scalar SQ, Scalar SV, CoordinateSystem TargetSpace, CoordinateSystem SourceSpace, bool IsRotationQuaternion>
	[[nodiscard]]
	constexpr auto
	operator* (Quaternion<SQ, TargetSpace, SourceSpace, IsRotationQuaternion> const& rotation,
			   Vector<SV, 3, SourceSpace, void> const& vector)
	{
		Quaternion<SV, SourceSpace, void, false> vector_quaternion (vector);
		auto const inv_rotation = coordinate_system_cast<void, SourceSpace> (rotation.inverted());
		return coordinate_system_cast<TargetSpace, void> (rotation * vector_quaternion * inv_rotation).imag();
	}


template<
	Scalar SQ,
	Scalar SM,
	std::size_t Columns,
	CoordinateSystem TargetSpace,
	CoordinateSystem IntermediateSpace,
	CoordinateSystem SourceSpace,
	bool IsRotationQuaternion
>
	[[nodiscard]]
	constexpr auto
	operator* (Quaternion<SQ, TargetSpace, IntermediateSpace, IsRotationQuaternion> const& rotation,
			   Matrix<SM, Columns, 3, IntermediateSpace, SourceSpace> const& matrix)
	{
		auto result = Matrix<SM, Columns, 3, TargetSpace, SourceSpace> (math::uninitialized);

		// Rotate each vector individually:
		for (std::size_t c = 0; c < matrix.n_columns(); ++c)
			result.put (rotation * matrix.column (c), c, 0);

		return result;
	}


template<Scalar S, CoordinateSystem TargetSpace, CoordinateSystem SourceSpace, bool IsRotationQuaternion>
	[[nodiscard]]
	constexpr auto
	operator/ (Quaternion<S, TargetSpace, SourceSpace, IsRotationQuaternion> const& q,
			   S const scalar)
	{
		return Quaternion<S, TargetSpace, SourceSpace, IsRotationQuaternion> (q.real() / scalar, q.imag() / scalar);
	}


template<Scalar S, CoordinateSystem TargetSpace, CoordinateSystem SourceSpace, bool IsRotationQuaternion>
	[[nodiscard]]
	constexpr auto
	operator/ (S const scalar,
			   Quaternion<S, TargetSpace, SourceSpace, IsRotationQuaternion> const& q)
	{
		return scalar * q.inverted();
	}


template<
	Scalar SA,
	Scalar SB,
	CoordinateSystem TargetSpace,
	CoordinateSystem IntermediateSpace,
	CoordinateSystem SourceSpace,
	bool IsRotationQuaternionA,
	bool IsRotationQuaternionB
>
	[[nodiscard]]
	constexpr auto
	operator/ (Quaternion<SA, TargetSpace, IntermediateSpace, IsRotationQuaternionA> const& a,
			   Quaternion<SB, SourceSpace, IntermediateSpace, IsRotationQuaternionB> const& b)
	{
		return a * b.inverted();
	}


/**
 * Return dot product of two quaternions.
 */
template<
	Scalar SA,
	Scalar SB,
	std::size_t Size,
	CoordinateSystem TargetSpace,
	CoordinateSystem SourceSpace,
	bool IsRotationQuaternionA,
	bool IsRotationQuaternionB
>
	[[nodiscard]]
	constexpr auto
	dot_product (Quaternion<SA, TargetSpace, SourceSpace, IsRotationQuaternionA> const& a,
				 Quaternion<SB, TargetSpace, SourceSpace, IsRotationQuaternionB> const& b)
		noexcept (noexcept (std::declval<SA>() * std::declval<SB>() + std::declval<SA>() * std::declval<SB>()))
	{
		return a.w() * b.w() + a.x() * b.x() + a.y() * b.y() + a.z() * b.z();
	}


template<Scalar S, CoordinateSystem TargetSpace, CoordinateSystem SourceSpace, bool IsRotationQuaternion>
	[[nodiscard]]
	constexpr auto
	abs (Quaternion<S, TargetSpace, SourceSpace, IsRotationQuaternion> const& q)
	{
		return q.norm();
	}


template<Scalar S, CoordinateSystem TargetSpace, CoordinateSystem SourceSpace, bool IsRotationQuaternion>
	[[nodiscard]]
	constexpr auto
	inv (Quaternion<S, TargetSpace, SourceSpace, IsRotationQuaternion> const& q)
	{
		return q.inverted();
	}

} // namespace neutrino::math

#endif

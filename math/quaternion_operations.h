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

template<class Scalar, class TargetSpace, class SourceSpace>
	constexpr Quaternion<Scalar, TargetSpace, SourceSpace>
	operator+ (Quaternion<Scalar, TargetSpace, SourceSpace> const q) noexcept
	{
		return q;
	}


template<class Scalar, class TargetSpace, class SourceSpace>
	constexpr auto
	operator+ (Quaternion<Scalar, TargetSpace, SourceSpace> const a,
			   Quaternion<Scalar, TargetSpace, SourceSpace> const& b)
		noexcept (noexcept (a.real() + b.real(), a.imag() + b.imag()))
	{
		return Quaternion (a.real() + b.real(), a.imag() + b.imag());
	}


template<class Scalar, class TargetSpace, class SourceSpace>
	constexpr Quaternion<Scalar, TargetSpace, SourceSpace>
	operator- (Quaternion<Scalar, TargetSpace, SourceSpace> const q)
		noexcept (noexcept (-std::declval<Scalar>()))
	{
		return Quaternion (-q.w(), -q.x(), -q.y(), -q.z());
	}


template<class Scalar, class TargetSpace, class SourceSpace>
	constexpr auto
	operator- (Quaternion<Scalar, TargetSpace, SourceSpace> const a,
			   Quaternion<Scalar, TargetSpace, SourceSpace> const& b)
		noexcept (noexcept (a.real() - b.real(), a.imag() - b.imag()))
	{
		return Quaternion (a.real() - b.real(), a.imag() - b.imag());
	}


template<class Scalar, class TargetSpace, class SourceSpace>
	constexpr auto
	operator* (Quaternion<Scalar, TargetSpace, SourceSpace> const q,
			   typename Quaternion<Scalar, TargetSpace, SourceSpace>::Scalar const& scalar)
		noexcept (noexcept (q.real() * scalar) && noexcept (q.imag() * scalar))
	{
		return Quaternion (q.real() * scalar, q.imag() * scalar);
	}


template<class Scalar, class TargetSpace, class SourceSpace>
	constexpr auto
	operator* (typename Quaternion<Scalar, TargetSpace, SourceSpace>::Scalar const& scalar,
			   Quaternion<Scalar, TargetSpace, SourceSpace> const q)
		noexcept (noexcept (q * scalar))
	{
		return q * scalar;
	}


template<class ScalarA, class ScalarB, class TargetSpace, class IntermediateSpace, class SourceSpace>
	constexpr auto
	operator* (Quaternion<ScalarA, TargetSpace, IntermediateSpace> const a,
			   Quaternion<ScalarB, IntermediateSpace, SourceSpace> const& b)
		noexcept (noexcept (a.w() * b.w() - a.x() * b.x() + a.y() * b.y()))
	{
		auto const w1 = a.w();	auto const w2 = b.w();
		auto const x1 = a.x();	auto const x2 = b.x();
		auto const y1 = a.y();	auto const y2 = b.y();
		auto const z1 = a.z();	auto const z2 = b.z();

		auto const w = w1 * w2 - x1 * x2 - y1 * y2 - z1 * z2;
		auto const x = w1 * x2 + x1 * w2 + y1 * z2 - z1 * y2;
		auto const y = w1 * y2 - x1 * z2 + y1 * w2 + z1 * x2;
		auto const z = w1 * z2 + x1 * y2 - y1 * x2 + z1 * w2;

		using ResultScalar = decltype (std::declval<ScalarA>() * std::declval<ScalarB>());
		return Quaternion<ResultScalar, TargetSpace, SourceSpace> (w, x, y, z);
	}


template<class ScalarQ, class ScalarV, class TargetSpace, class SourceSpace>
	constexpr auto
	operator* (Quaternion<ScalarQ, TargetSpace, SourceSpace> const& rotation,
			   Vector<ScalarV, 3, SourceSpace, void> const& vector)
	{
		Quaternion<ScalarV, SourceSpace, void> vector_quaternion (vector);
		// Assuming we're doing rotation here, so conjugating. In reality it should be inversion.
		// TODO Maybe have a separate type RotationQuaternion that does the conjugate?
		auto const conj_rotation = reframe<void, SourceSpace> (~rotation);
		return reframe<TargetSpace, void> (rotation * vector_quaternion * conj_rotation).imag();
	}


template<class Scalar, class TargetSpace, class SourceSpace>
	constexpr auto
	operator/ (Quaternion<Scalar, TargetSpace, SourceSpace> const q,
			   typename Quaternion<Scalar, TargetSpace, SourceSpace>::Scalar const& scalar)
	{
		return Quaternion (q.real() / scalar, q.imag() / scalar);
	}


template<class Scalar, class TargetSpace, class SourceSpace>
	constexpr auto
	operator/ (typename Quaternion<Scalar, TargetSpace, SourceSpace>::Scalar const& scalar,
			   Quaternion<Scalar, TargetSpace, SourceSpace> const& q)
	{
		return scalar * q.inverted();
	}


template<class Scalar, class TargetSpace, class IntermediateSpace, class SourceSpace>
	constexpr auto
	operator/ (Quaternion<Scalar, TargetSpace, IntermediateSpace> const a,
			   Quaternion<Scalar, SourceSpace, IntermediateSpace> const& b)
	{
		return a * b.inverted();
	}


template<class Scalar, class TargetSpace, class SourceSpace>
	constexpr auto
	inv (Quaternion<Scalar, TargetSpace, SourceSpace> const& q)
	{
		return q.inverted();
	}

} // namespace neutrino::math

#endif


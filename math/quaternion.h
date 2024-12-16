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

#ifndef NEUTRINO__MATH__QUATERNION_H__INCLUDED
#define NEUTRINO__MATH__QUATERNION_H__INCLUDED

// Local:
#include <neutrino/math/concepts.h>
#include <neutrino/math/matrix.h>
#include <neutrino/math/matrix_operations.h>

// Standard:
#include <cmath>
#include <cstddef>
#include <array>
#include <type_traits>
#include <utility>
#include <vector>


namespace neutrino::math {

class BasicQuaternion
{ };


template<Scalar pScalar, CoordinateSystem pTargetSpace = void, CoordinateSystem pSourceSpace = pTargetSpace>
	class Quaternion: public BasicQuaternion
	{
	  public:
		using Scalar		= pScalar;
		using SquaredScalar	= std::remove_cvref_t<decltype (std::declval<Scalar>() * std::declval<Scalar>())>;
		using TargetSpace	= pTargetSpace;
		using SourceSpace	= pSourceSpace;

	  public:
		// Ctor, initializes to zero.
		constexpr
		Quaternion();

		// Copy ctor
		constexpr
		Quaternion (Quaternion const&) = default;

		// Move ctor
		constexpr
		Quaternion (Quaternion&&) = default;

		// Ctor. Alias for default constructor (initialized to zero).
		constexpr
		Quaternion (ZeroInitializer) noexcept;

		// Ctor. Initializes to identity quaternion.
		constexpr
		Quaternion (UnitInitializer) noexcept;

		// Ctor. Initializes to identity quaternion.
		constexpr
		Quaternion (IdentityInitializer) noexcept;

		// Ctor. Doesn't initialize the quaternion at all if Scalar is a fundamental type; otherwise equivalent to ZeroInitializer.
		constexpr
		Quaternion (UninitializedInitializer) noexcept;

		/**
		 * Constructs pure quaternion.
		 */
		constexpr
		Quaternion (Vector<Scalar, 3, TargetSpace, void> const&);

		/**
		 * Constructs proper quaternion.
		 */
		constexpr
		Quaternion (Vector<Scalar, 4, TargetSpace, SourceSpace> const&);

		// Ctor
		constexpr
		Quaternion (std::array<Scalar, 4> const&);

		// Ctor
		constexpr
		Quaternion (Scalar w, Scalar x, Scalar y, Scalar z);

		// Ctor
		constexpr
		Quaternion (Scalar w, Vector<Scalar, 3, TargetSpace, void> const&);

		// Ctor
		explicit constexpr
		Quaternion (Matrix<Scalar, 3, 3, TargetSpace, SourceSpace> const&);

		// Copy operator
		constexpr Quaternion&
		operator= (Quaternion const&) noexcept (std::is_copy_assignable_v<Scalar>) = default;

		// Move operator
		constexpr Quaternion&
		operator= (Quaternion&&) noexcept (std::is_move_assignable_v<Scalar>) = default;

		/**
		 * Equality operator
		 */
		[[nodiscard]]
		constexpr bool
		operator== (Quaternion const&) const = default;

		/**
		 * Difference operator
		 */
		[[nodiscard]]
		constexpr bool
		operator!= (Quaternion const&) const = default;

		/**
		 * Return the rotation value.
		 */
		[[nodiscard]]
		Scalar&
		w() noexcept
			{ return _components[0]; }

		/**
		 * Return the rotation value.
		 */
		[[nodiscard]]
		Scalar const&
		w() const noexcept
			{ return _components[0]; }

		/**
		 * Return the x position.
		 */
		[[nodiscard]]
		Scalar&
		x() noexcept
			{ return _components[1]; };

		/**
		 * Return the x position.
		 */
		[[nodiscard]]
		Scalar const&
		x() const noexcept
			{ return _components[1]; }

		/**
		 * Return the y position.
		 */
		[[nodiscard]]
		Scalar&
		y() noexcept { return _components[2]; }

		/**
		 * Return the y position.
		 */
		[[nodiscard]]
		Scalar const&
		y() const noexcept { return _components[2]; };

		/**
		 * Return the z position.
		 */
		[[nodiscard]]
		Scalar&
		z() noexcept { return _components[3]; };

		/**
		 * Return the z position.
		 */
		[[nodiscard]]
		Scalar const&
		z() const noexcept { return _components[3]; };

		/**
		 * Return the array of components { w, x, y, z }.
		 */
		[[nodiscard]]
		constexpr std::array<Scalar, 4> const&
		components() const noexcept
			{ return _components; }

		/**
		 * Real component (aka scalar component). Alias for w().
		 */
		[[nodiscard]]
		constexpr Scalar
		real() const noexcept
			{ return w(); }

		/**
		 * Return the vector of imaginary components.
		 */
		[[nodiscard]]
		constexpr Vector<Scalar, 3, TargetSpace, void>
		imag() const noexcept
			{ return { x(), y(), z() }; }

		/**
		 * Return quaternion's norm.
		 */
		[[nodiscard]]
		constexpr Scalar
		norm() const noexcept;

		/**
		 * Return squared norm.
		 */
		[[nodiscard]]
		constexpr SquaredScalar
		squared_norm() const noexcept;

		/**
		 * Normalize this quaternion inline.
		 */
		constexpr void
		normalize()
			{ *this /= (1 / Scalar (1)) * norm(); }

		/**
		 * Return new, normalized quaternion.
		 */
		[[nodiscard]]
		constexpr Quaternion
		normalized() const
			{ return Scalar (1) * *this / norm(); }

		/**
		 * Conjugate this quaternion.
		 */
		constexpr void
		conjugate();

		/**
		 * Return new, conjugated quaternion.
		 */
		[[nodiscard]]
		constexpr Quaternion<Scalar, SourceSpace, TargetSpace>
		conjugated() const
			{ return { w(), -x(), -y(), -z() }; }

		/**
		 * Invert this quaternion.
		 */
		constexpr void
		invert();

		/**
		 * Return inverse of this quaternion.
		 */
		[[nodiscard]]
		constexpr Quaternion<Scalar, SourceSpace, TargetSpace>
		inverted() const
			{ return conjugated() / (1 / SquaredScalar (1)) * squared_norm(); }

		/**
		 * Alias for conjugated().
		 */
		[[nodiscard]]
		constexpr Quaternion<Scalar, SourceSpace, TargetSpace>
		operator~() const noexcept
			{ return conjugated(); }

		/**
		 * Add another quaternion to this one.
		 */
		constexpr Quaternion&
		operator+= (Quaternion const& other)
			noexcept (noexcept (*this + other))
		{ return *this = *this + other; }

		/**
		 * Subtract another quaternion from this one.
		 */
		constexpr Quaternion&
		operator-= (Quaternion const& other)
			noexcept (noexcept (*this - other))
		{ return *this = *this - other; }

		/**
		 * Multiply this quaternion by a scalar.
		 */
		constexpr Quaternion&
		operator*= (Scalar const& other)
			noexcept (noexcept (*this * other))
		{ return *this = *this * other; }

		/**
		 * Multiply this quaternion by another quaternion.
		 */
		constexpr Quaternion&
		operator*= (Quaternion const& other)
			noexcept (noexcept (*this * other))
		{ return *this = *this * other; }

		/**
		 * Divide this quaternion by a scalar.
		 */
		constexpr Quaternion&
		operator/= (Scalar const& other)
			noexcept (noexcept (*this / other))
		{ return *this = *this / other; }

		/**
		 * Divide this quaternion by by another quaternion.
		 */
		constexpr Quaternion&
		operator/= (Quaternion const& other)
			noexcept (noexcept (*this / other))
		{ return *this = *this / other; }

	  private:
		alignas(8) std::array<Scalar, 4> _components;
	};


template<Scalar S, CoordinateSystem TS, CoordinateSystem SS>
	constexpr
	Quaternion<S, TS, SS>::Quaternion():
		Quaternion (math::zero)
	{ }


template<Scalar S, CoordinateSystem TS, CoordinateSystem SS>
	constexpr
	Quaternion<S, TS, SS>::Quaternion (ZeroInitializer) noexcept:
		_components { Scalar{0}, Scalar{0}, Scalar{0}, Scalar{0} }
	{ }


template<Scalar S, CoordinateSystem TS, CoordinateSystem SS>
	constexpr
	Quaternion<S, TS, SS>::Quaternion (UnitInitializer) noexcept:
		Quaternion (math::identity)
	{ }


template<Scalar S, CoordinateSystem TS, CoordinateSystem SS>
	constexpr
	Quaternion<S, TS, SS>::Quaternion (IdentityInitializer) noexcept:
		_components { Scalar{1}, Scalar{0}, Scalar{0}, Scalar{0} }
	{ }


template<Scalar S, CoordinateSystem TS, CoordinateSystem SS>
	constexpr
	Quaternion<S, TS, SS>::Quaternion (UninitializedInitializer) noexcept
	{ }


template<Scalar S, CoordinateSystem TS, CoordinateSystem SS>
	constexpr
	Quaternion<S, TS, SS>::Quaternion (Vector<Scalar, 3, TargetSpace, void> const& vector):
		_components ({ Scalar (0), vector[0], vector[1], vector[2] })
	{ }


template<Scalar S, CoordinateSystem TS, CoordinateSystem SS>
	constexpr
	Quaternion<S, TS, SS>::Quaternion (Vector<Scalar, 4, TargetSpace, SourceSpace> const& vector):
		_components ({ vector[0], vector[1], vector[2], vector[3] })
	{ }


template<Scalar S, CoordinateSystem TS, CoordinateSystem SS>
	constexpr
	Quaternion<S, TS, SS>::Quaternion (std::array<Scalar, 4> const& array):
		_components (array)
	{ }


template<Scalar S, CoordinateSystem TS, CoordinateSystem SS>
	constexpr
	Quaternion<S, TS, SS>::Quaternion (Scalar w, Scalar x, Scalar y, Scalar z):
		_components { w, x, y, z }
	{ }


template<Scalar S, CoordinateSystem TS, CoordinateSystem SS>
	constexpr
	Quaternion<S, TS, SS>::Quaternion (Scalar w, Vector<Scalar, 3, TargetSpace, void> const& vector):
		_components { w, vector[0], vector[1], vector[2] }
	{ }


template<Scalar S, CoordinateSystem TS, CoordinateSystem SS>
	constexpr
	Quaternion<S, TS, SS>::Quaternion (Matrix<Scalar, 3, 3, TargetSpace, SourceSpace> const& m)
	{
		S t { 0 };
		auto const m00 = m[0, 0];
		auto const m01 = m[0, 1];
		auto const m02 = m[0, 2];
		auto const m10 = m[1, 0];
		auto const m11 = m[1, 1];
		auto const m12 = m[1, 2];
		auto const m20 = m[2, 0];
		auto const m21 = m[2, 1];
		auto const m22 = m[2, 2];

		if (m22 < 0)
		{
			if (m00 > m11)
			{
				t = 1 + m[0, 0] - m11 - m22;
				x() = t;
				y() = m01 + m10;
				z() = m20 + m02;
				w() = m12 - m21;
			}
			else
			{
				t = 1 - m00 + m11 - m22;
				x() = m01 + m10;
				y() = t;
				z() = m12 + m21;
				w() = m20 - m02;
			}
		}
		else
		{
			if (m00 < -m11)
			{
				t = 1 - m00 - m11 + m22;
				x() = m20 + m02;
				y() = m12 + m21;
				z() = t;
				w() = m01 - m10;
			}
			else
			{
				t = 1 + m00 + m11 + m22;
				x() = m12 - m21;
				y() = m20 - m02;
				z() = m01 - m10;
				w() = t;
			}
		}

		*this *= 0.5 / std::sqrt (t);
	}


template<Scalar S, CoordinateSystem TS, CoordinateSystem SS>
	constexpr auto
	Quaternion<S, TS, SS>::norm() const noexcept
		-> Scalar
	{
		using std::sqrt;
		return sqrt (squared_norm());
	}


template<Scalar S, CoordinateSystem TS, CoordinateSystem SS>
	constexpr auto
	Quaternion<S, TS, SS>::squared_norm() const noexcept
		-> SquaredScalar
	{
		SquaredScalar sum { 0 };

		for (auto c: _components)
			sum += c * c;

		return sum;
	}


template<Scalar S, CoordinateSystem TS, CoordinateSystem SS>
	constexpr void
	Quaternion<S, TS, SS>::conjugate()
	{
		static_assert (std::is_same_v<TS, SS>, "in-place conjugation requires same TargetSpace and SourceSpace");

		x() = -x();
		y() = -y();
		z() = -z();
	}


template<Scalar S, CoordinateSystem TS, CoordinateSystem SS>
	constexpr void
	Quaternion<S, TS, SS>::invert()
	{
		static_assert (std::is_same_v<TS, SS>, "in-place inversion requires same TargetSpace and SourceSpace");

		conjugate();
		*this /= (1 / SquaredScalar (1)) * squared_norm();
	}


template<CoordinateSystem NewTargetSpace, CoordinateSystem NewSourceSpace, Scalar S, CoordinateSystem OldTargetSpace, CoordinateSystem OldSourceSpace>
	[[nodiscard]]
	constexpr auto&
	coordinate_system_cast (Quaternion<S, OldTargetSpace, OldSourceSpace>& quaternion)
	{
		using Scalar = typename std::remove_cvref_t<decltype (quaternion)>::Scalar;
		return reinterpret_cast<Quaternion<Scalar, NewTargetSpace, NewSourceSpace>&> (quaternion);
	}


template<CoordinateSystem NewTargetSpace, CoordinateSystem NewSourceSpace, Scalar S, CoordinateSystem OldTargetSpace, CoordinateSystem OldSourceSpace>
	[[nodiscard]]
	constexpr auto const&
	coordinate_system_cast (Quaternion<S, OldTargetSpace, OldSourceSpace> const& quaternion)
	{
		using Scalar = typename std::remove_cvref_t<decltype (quaternion)>::Scalar;
		return reinterpret_cast<Quaternion<Scalar, NewTargetSpace, NewSourceSpace> const&> (quaternion);
	}

} // namespace neutrino::math


// Local:
#include "quaternion_operations.h"

#endif


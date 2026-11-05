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
#include <neutrino/math/norm.h>

// Standard:
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <array>
#include <type_traits>
#include <utility>
#include <vector>


namespace neutrino::math {

class BasicQuaternion
{ };


/**
 * Convert each component using static_cast<NewScalar>(), preserving coordinate tags and the
 * unit-norm flag. Rotation quaternions are renormalized after narrowing or changing stored units.
 */
template<class NewScalar, Scalar OldScalar, CoordinateSystem TargetSpace, CoordinateSystem SourceSpace, bool IsRotationQuaternion>
	[[nodiscard]]
	constexpr Quaternion<NewScalar, TargetSpace, SourceSpace, IsRotationQuaternion>
	static_components_cast (Quaternion<OldScalar, TargetSpace, SourceSpace, IsRotationQuaternion> const& quaternion)
		requires (Scalar<NewScalar> && requires (OldScalar const& value) { static_cast<NewScalar> (value); });


template<
	Scalar pScalar,
	CoordinateSystem pTargetSpace,
	CoordinateSystem pSourceSpace,
	bool pIsRotationQuaternion
>
	class Quaternion: public BasicQuaternion
	{
	  public:
		using Scalar		= pScalar;
		using SquaredScalar	= std::remove_cvref_t<decltype (std::declval<Scalar>() * std::declval<Scalar>())>;
		using InverseScalar	= std::remove_cvref_t<decltype (1 / std::declval<Scalar>())>;
		using TargetSpace	= pTargetSpace;
		using SourceSpace	= pSourceSpace;
		// Unit-norm guarantee used by inverted(); norm-changing arithmetic must discard it.
		// When set, callers supplying or modifying components must ensure unit norm before use.
		static constexpr bool kIsRotationQuaternion = pIsRotationQuaternion;

		template<class NewScalar>
			using Retyped = Quaternion<NewScalar, TargetSpace, SourceSpace, kIsRotationQuaternion>;

	  public:
		// Ctor. Initializes rotations to identity and general quaternions to zero.
		constexpr
		Quaternion();

		// Copy ctor
		constexpr
		Quaternion (Quaternion const&) = default;

		// Move ctor
		constexpr
		Quaternion (Quaternion&&) noexcept (std::is_nothrow_move_constructible_v<Scalar>) = default;

		// Allow discarding the unit-norm guarantee, but never implicitly introduce it.
		template<bool OtherIsRotationQuaternion>
			requires (!kIsRotationQuaternion && OtherIsRotationQuaternion)
			constexpr
			Quaternion (Quaternion<Scalar, TargetSpace, SourceSpace, OtherIsRotationQuaternion> const& other)
				noexcept (std::is_nothrow_copy_constructible_v<Scalar>):
				_components (other.components())
			{ }

		// Ctor. Initializes to zero; unavailable for rotations because zero has no unit norm.
		constexpr
		Quaternion (ZeroInitializer) noexcept
			requires (!kIsRotationQuaternion);

		// Ctor. Initializes to identity quaternion.
		constexpr
		Quaternion (UnitInitializer) noexcept;

		// Ctor. Initializes to identity quaternion.
		constexpr
		Quaternion (IdentityInitializer) noexcept;

		// Ctor. Leaves fundamental components uninitialized; otherwise default-constructs each component.
		// The caller must initialize all components before use, ensuring unit norm for rotations.
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
		 * Allow scalar conversions that do not narrow, as for Matrix.
		 * Coordinate tags and the unit-norm flag must remain unchanged.
		 */
		template<class OtherScalar>
			requires (!std::same_as<Scalar, OtherScalar> &&
					  neutrino::math::Scalar<OtherScalar> &&
					  detail::ImplicitlyNonNarrowingConvertible<Scalar, OtherScalar>)
			[[nodiscard]]
			constexpr
			operator Retyped<OtherScalar>() const
				{ return static_components_cast<OtherScalar> (*this); }

		/**
		 * Compare quaternion components exactly.
		 */
		[[nodiscard]]
		constexpr bool
		operator== (Quaternion const& other) const
			{ return _components == other._components; }

		/**
		 * Return the negation of component equality.
		 */
		[[nodiscard]]
		constexpr bool
		operator!= (Quaternion const& other) const
			{ return !(*this == other); }

		/**
		 * Return the rotation value.
		 */
		[[nodiscard]]
		constexpr Scalar&
		w() noexcept
			{ return _components[0]; }

		/**
		 * Return the rotation value.
		 */
		[[nodiscard]]
		constexpr Scalar const&
		w() const noexcept
			{ return _components[0]; }

		/**
		 * Return the x position.
		 */
		[[nodiscard]]
		constexpr Scalar&
		x() noexcept
			{ return _components[1]; };

		/**
		 * Return the x position.
		 */
		[[nodiscard]]
		constexpr Scalar const&
		x() const noexcept
			{ return _components[1]; }

		/**
		 * Return the y position.
		 */
		[[nodiscard]]
		constexpr Scalar&
		y() noexcept { return _components[2]; }

		/**
		 * Return the y position.
		 */
		[[nodiscard]]
		constexpr Scalar const&
		y() const noexcept { return _components[2]; };

		/**
		 * Return the z position.
		 */
		[[nodiscard]]
		constexpr Scalar&
		z() noexcept { return _components[3]; };

		/**
		 * Return the z position.
		 */
		[[nodiscard]]
		constexpr Scalar const&
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
		 * Return quaternion's norm without overflow or underflow from squaring components.
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
		 * Normalize this quaternion in place.
		 * Scale finite components before computing their norm, preserving Scalar's stored unit.
		 */
		constexpr void
		normalize();

		/**
		 * Return new, normalized quaternion.
		 */
		[[nodiscard]]
		constexpr Quaternion
		normalized() const;

		/**
		 * Conjugate this quaternion.
		 */
		constexpr void
		conjugate();

		/**
		 * Return new, conjugated quaternion.
		 */
		[[nodiscard]]
		constexpr Quaternion<Scalar, SourceSpace, TargetSpace, kIsRotationQuaternion>
		conjugated() const
			{ return { w(), -x(), -y(), -z() }; }

		/**
		 * Invert this quaternion.
		 * Available only when the inverse's units and coordinate tags can be assigned back.
		 */
		constexpr void
		invert()
			requires (std::is_same_v<TargetSpace, SourceSpace> && std::is_assignable_v<Scalar&, InverseScalar>);

		/**
		 * Return inverse of this quaternion.
		 * Component units are inverted and coordinate-system tags are swapped.
		 * Finite components are scaled before squaring to avoid overflow and underflow.
		 */
		[[nodiscard]]
		constexpr Quaternion<InverseScalar, SourceSpace, TargetSpace, kIsRotationQuaternion>
		inverted() const;

		/**
		 * Alias for conjugated().
		 */
		[[nodiscard]]
		constexpr Quaternion<Scalar, SourceSpace, TargetSpace, kIsRotationQuaternion>
		operator~() const noexcept
			{ return conjugated(); }

		/**
		 * Add another quaternion to this one.
		 * Unavailable for rotation quaternions because addition does not preserve unit norm.
		 */
		constexpr Quaternion&
		operator+= (Quaternion const& other)
			noexcept (noexcept (*this + other))
			requires (!kIsRotationQuaternion)
		{ return *this = *this + other; }

		/**
		 * Subtract another quaternion from this one.
		 * Unavailable for rotation quaternions because subtraction does not preserve unit norm.
		 */
		constexpr Quaternion&
		operator-= (Quaternion const& other)
			noexcept (noexcept (*this - other))
			requires (!kIsRotationQuaternion)
		{ return *this = *this - other; }

		/**
		 * Multiply this quaternion by a scalar.
		 * Unavailable for rotation quaternions because scaling does not preserve unit norm.
		 * The resulting components must be assignable to Scalar.
		 */
		template<class Factor>
			requires (!kIsRotationQuaternion && (math::Scalar<Factor> || std::integral<Factor>) &&
					  requires (Scalar& component, Factor const& factor) { component = component * factor; })
			constexpr Quaternion&
			operator*= (Factor const& other)
				noexcept (noexcept (w() = w() * other))
			{ return *this = Quaternion (w() * other, x() * other, y() * other, z() * other); }

		/**
		 * Right-compose with a quaternion acting within SourceSpace.
		 * The resulting components must be assignable to Scalar; rotations require a rotation operand.
		 */
		template<math::Scalar OtherScalar, bool OtherIsRotationQuaternion>
			requires ((!kIsRotationQuaternion || OtherIsRotationQuaternion) &&
					  requires (Scalar& component, OtherScalar const& factor) { component = component * factor; })
			constexpr Quaternion&
			operator*= (Quaternion<OtherScalar, SourceSpace, SourceSpace, OtherIsRotationQuaternion> const& other)
				noexcept (noexcept (*this * other))
			{
				auto const result = *this * other;
				return *this = Quaternion (result.w(), result.x(), result.y(), result.z());
			}

		/**
		 * Divide this quaternion by a scalar.
		 * Unavailable for rotation quaternions because scaling does not preserve unit norm.
		 * The resulting components must be assignable to Scalar.
		 */
		template<class Factor>
			requires (!kIsRotationQuaternion && (math::Scalar<Factor> || std::integral<Factor>) &&
					  requires (Scalar& component, Factor const& factor) { component = component / factor; })
			constexpr Quaternion&
			operator/= (Factor const& other)
				noexcept (noexcept (w() = w() / other))
			{ return *this = Quaternion (w() / other, x() / other, y() / other, z() / other); }

		/**
		 * Right-compose with the inverse of a quaternion acting within SourceSpace.
		 * The resulting components must be assignable to Scalar; rotations require a rotation operand.
		 */
		template<math::Scalar OtherScalar, bool OtherIsRotationQuaternion>
			requires ((!kIsRotationQuaternion || OtherIsRotationQuaternion) &&
					  requires (Scalar& component, OtherScalar const& factor) { component = component * (1 / factor); })
			constexpr Quaternion&
			operator/= (Quaternion<OtherScalar, SourceSpace, SourceSpace, OtherIsRotationQuaternion> const& other)
				noexcept (noexcept (*this / other))
			{
				auto const result = *this / other;
				return *this = Quaternion (result.w(), result.x(), result.y(), result.z());
			}

	  private:
		alignas(8) std::array<Scalar, 4> _components;
	};


template<Scalar S, CoordinateSystem TS, CoordinateSystem SS, bool IsRotationQuaternion>
	constexpr
	Quaternion<S, TS, SS, IsRotationQuaternion>::Quaternion():
		_components { Scalar (kIsRotationQuaternion ? 1 : 0), Scalar (0), Scalar (0), Scalar (0) }
	{ }


template<Scalar S, CoordinateSystem TS, CoordinateSystem SS, bool IsRotationQuaternion>
	constexpr
	Quaternion<S, TS, SS, IsRotationQuaternion>::Quaternion (ZeroInitializer) noexcept
		requires (!kIsRotationQuaternion):
		_components { Scalar{0}, Scalar{0}, Scalar{0}, Scalar{0} }
	{ }


template<Scalar S, CoordinateSystem TS, CoordinateSystem SS, bool IsRotationQuaternion>
	constexpr
	Quaternion<S, TS, SS, IsRotationQuaternion>::Quaternion (UnitInitializer) noexcept:
		Quaternion (math::identity)
	{ }


template<Scalar S, CoordinateSystem TS, CoordinateSystem SS, bool IsRotationQuaternion>
	constexpr
	Quaternion<S, TS, SS, IsRotationQuaternion>::Quaternion (IdentityInitializer) noexcept:
		_components { Scalar{1}, Scalar{0}, Scalar{0}, Scalar{0} }
	{ }


template<Scalar S, CoordinateSystem TS, CoordinateSystem SS, bool IsRotationQuaternion>
	constexpr
	Quaternion<S, TS, SS, IsRotationQuaternion>::Quaternion (UninitializedInitializer) noexcept
	{ }


template<Scalar S, CoordinateSystem TS, CoordinateSystem SS, bool IsRotationQuaternion>
	constexpr
	Quaternion<S, TS, SS, IsRotationQuaternion>::Quaternion (Vector<Scalar, 3, TargetSpace, void> const& vector):
		_components ({ Scalar (0), vector[0], vector[1], vector[2] })
	{ }


template<Scalar S, CoordinateSystem TS, CoordinateSystem SS, bool IsRotationQuaternion>
	constexpr
	Quaternion<S, TS, SS, IsRotationQuaternion>::Quaternion (Vector<Scalar, 4, TargetSpace, SourceSpace> const& vector):
		_components ({ vector[0], vector[1], vector[2], vector[3] })
	{ }


template<Scalar S, CoordinateSystem TS, CoordinateSystem SS, bool IsRotationQuaternion>
	constexpr
	Quaternion<S, TS, SS, IsRotationQuaternion>::Quaternion (std::array<Scalar, 4> const& array):
		_components (array)
	{ }


template<Scalar S, CoordinateSystem TS, CoordinateSystem SS, bool IsRotationQuaternion>
	constexpr
	Quaternion<S, TS, SS, IsRotationQuaternion>::Quaternion (Scalar w, Scalar x, Scalar y, Scalar z):
		_components { w, x, y, z }
	{ }


template<Scalar S, CoordinateSystem TS, CoordinateSystem SS, bool IsRotationQuaternion>
	constexpr
	Quaternion<S, TS, SS, IsRotationQuaternion>::Quaternion (Scalar w, Vector<Scalar, 3, TargetSpace, void> const& vector):
		_components { w, vector[0], vector[1], vector[2] }
	{ }


template<Scalar S, CoordinateSystem TS, CoordinateSystem SS, bool IsRotationQuaternion>
	constexpr
	Quaternion<S, TS, SS, IsRotationQuaternion>::Quaternion (Matrix<Scalar, 3, 3, TargetSpace, SourceSpace> const& m)
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

		auto const scale = 0.5 / std::sqrt (t);

		for (auto& component: _components)
			component *= scale;
	}


template<Scalar S, CoordinateSystem TS, CoordinateSystem SS, bool IsRotationQuaternion>
	constexpr auto
	Quaternion<S, TS, SS, IsRotationQuaternion>::norm() const noexcept
		-> Scalar
	{
		return detail::stable_norm (_components);
	}


template<Scalar S, CoordinateSystem TS, CoordinateSystem SS, bool IsRotationQuaternion>
	constexpr auto
	Quaternion<S, TS, SS, IsRotationQuaternion>::squared_norm() const noexcept
		-> SquaredScalar
	{
		SquaredScalar sum { 0 };

		for (auto c: _components)
			sum += c * c;

		return sum;
	}


template<Scalar S, CoordinateSystem TS, CoordinateSystem SS, bool IsRotationQuaternion>
	constexpr void
	Quaternion<S, TS, SS, IsRotationQuaternion>::normalize()
	{
		using Value = si::decay_quantity_t<Scalar>;
		auto const max_component = std::max ({
			std::abs (si::value (w())),
			std::abs (si::value (x())),
			std::abs (si::value (y())),
			std::abs (si::value (z()))
		});

		// Normalize the scaled components so an unrepresentable norm is never needed:
		if (max_component > Value (0) && std::isfinite (max_component))
		{
			for (auto& component: _components)
				component /= max_component;
		}

		auto const scale = si::value (norm());

		for (auto& component: _components)
			component /= scale;
	}


template<Scalar S, CoordinateSystem TS, CoordinateSystem SS, bool IsRotationQuaternion>
	constexpr auto
	Quaternion<S, TS, SS, IsRotationQuaternion>::normalized() const
		-> Quaternion
	{
		auto result = *this;
		result.normalize();
		return result;
	}


template<Scalar S, CoordinateSystem TS, CoordinateSystem SS, bool IsRotationQuaternion>
	constexpr void
	Quaternion<S, TS, SS, IsRotationQuaternion>::conjugate()
	{
		static_assert (std::is_same_v<TS, SS>, "in-place conjugation requires same TargetSpace and SourceSpace");

		x() = -x();
		y() = -y();
		z() = -z();
	}


template<Scalar S, CoordinateSystem TS, CoordinateSystem SS, bool IsRotationQuaternion>
	constexpr auto
	Quaternion<S, TS, SS, IsRotationQuaternion>::inverted() const
		-> Quaternion<InverseScalar, SourceSpace, TargetSpace, kIsRotationQuaternion>
	{
		using Inverse = Quaternion<InverseScalar, SourceSpace, TargetSpace, kIsRotationQuaternion>;

		// Quantity components also need their units inverted, even for rotation quaternions:
		if constexpr (kIsRotationQuaternion && std::floating_point<Scalar>)
			return conjugated();
		else
		{
			using Value = si::decay_quantity_t<Scalar>;
			auto const magnitude = std::max ({
				std::abs (si::value (w())),
				std::abs (si::value (x())),
				std::abs (si::value (y())),
				std::abs (si::value (z()))
			});

			if (magnitude > Value (0) && std::isfinite (magnitude))
			{
				auto const scale = Scalar (magnitude);
				auto const scaled = *this / scale;
				auto const squared_norm = scaled.squared_norm();

				// Combine small divisors to avoid losing subnormal components before rescaling:
				if (magnitude < Value (1))
				{
					auto const result = scaled.conjugated() / (squared_norm * scale);
					return Inverse (result.w(), result.x(), result.y(), result.z());
				}

				// Divide separately so the denominator cannot overflow for large components:
				auto const result = (scaled.conjugated() / squared_norm) / scale;
				return Inverse (result.w(), result.x(), result.y(), result.z());
			}

			auto const result = conjugated() / squared_norm();
			return Inverse (result.w(), result.x(), result.y(), result.z());
		}
	}


template<Scalar S, CoordinateSystem TS, CoordinateSystem SS, bool IsRotationQuaternion>
	constexpr void
	Quaternion<S, TS, SS, IsRotationQuaternion>::invert()
		requires (std::is_same_v<TargetSpace, SourceSpace> && std::is_assignable_v<Scalar&, InverseScalar>)
	{
		auto const inverse = inverted();
		*this = Quaternion (inverse.w(), inverse.x(), inverse.y(), inverse.z());
	}


/**
 * Return a copy with new coordinate-system tags, preserving components and the unit-norm flag.
 * Source coordinate-system tags must be specified explicitly.
 */
template<
	CoordinateSystem NewTargetSpace,
	CoordinateSystem NewSourceSpace,
	CoordinateSystem OldTargetSpace,
	CoordinateSystem OldSourceSpace,
	Scalar S,
	bool IsRotationQuaternion
>
	[[nodiscard]]
	constexpr auto
	coordinate_system_cast (Quaternion<S, std::type_identity_t<OldTargetSpace>, std::type_identity_t<OldSourceSpace>, IsRotationQuaternion> const& quaternion)
	{
		return Quaternion<S, NewTargetSpace, NewSourceSpace, IsRotationQuaternion> { quaternion.w(), quaternion.x(), quaternion.y(), quaternion.z() };
	}


/**
 * Return a copy with new coordinate-system tags, deducing the source tags.
 */
template<
	CoordinateSystem NewTargetSpace,
	CoordinateSystem NewSourceSpace,
	CoordinateSystem OldTargetSpace,
	CoordinateSystem OldSourceSpace,
	Scalar S,
	bool IsRotationQuaternion
>
	[[nodiscard]]
	constexpr auto
	coordinate_system_cast_any (Quaternion<S, OldTargetSpace, OldSourceSpace, IsRotationQuaternion> const& quaternion)
	{
		return coordinate_system_cast<NewTargetSpace, NewSourceSpace, OldTargetSpace, OldSourceSpace> (quaternion);
	}


template<class NewScalar, Scalar OldScalar, CoordinateSystem TargetSpace, CoordinateSystem SourceSpace, bool IsRotationQuaternion>
	[[nodiscard]]
	constexpr Quaternion<NewScalar, TargetSpace, SourceSpace, IsRotationQuaternion>
	static_components_cast (Quaternion<OldScalar, TargetSpace, SourceSpace, IsRotationQuaternion> const& quaternion)
		requires (Scalar<NewScalar> && requires (OldScalar const& value) { static_cast<NewScalar> (value); })
	{
		auto result = Quaternion<NewScalar, TargetSpace, SourceSpace, IsRotationQuaternion> {
			static_cast<NewScalar> (quaternion.w()),
			static_cast<NewScalar> (quaternion.x()),
			static_cast<NewScalar> (quaternion.y()),
			static_cast<NewScalar> (quaternion.z())
		};

		// Widening floating-point components preserves their values. Narrowing can change the
		// norm, and converting quantities can change the stored scale even without narrowing.
		if constexpr (IsRotationQuaternion && !std::same_as<OldScalar, NewScalar> &&
					  (!detail::ImplicitlyNonNarrowingConvertible<OldScalar, NewScalar> ||
					   !std::floating_point<OldScalar> || !std::floating_point<NewScalar>))
		{
			result.normalize();
		}

		return result;
	}


/**
 * Explicitly convert the component type, including narrowing conversions.
 * Preserve coordinate tags and the unit-norm flag, renormalizing rotations as needed.
 * Example: auto narrowed = narrow_cast<float> (quaternion);
 */
template<class NewScalar, Scalar OldScalar, CoordinateSystem TargetSpace, CoordinateSystem SourceSpace, bool IsRotationQuaternion>
	[[nodiscard]]
	constexpr Quaternion<NewScalar, TargetSpace, SourceSpace, IsRotationQuaternion>
	narrow_cast (Quaternion<OldScalar, TargetSpace, SourceSpace, IsRotationQuaternion> const& quaternion)
		requires (Scalar<NewScalar> && requires (OldScalar const& value) { static_cast<NewScalar> (value); })
	{
		return static_components_cast<NewScalar> (quaternion);
	}


/**
 * Convert to an exact quaternion type, changing only the component type.
 * Example: auto narrowed = narrow_cast<Quaternion<float>> (quaternion);
 */
template<class TargetQuaternion, Scalar OldScalar, CoordinateSystem TargetSpace, CoordinateSystem SourceSpace, bool IsRotationQuaternion>
	[[nodiscard]]
	constexpr std::remove_cvref_t<TargetQuaternion>
	narrow_cast (Quaternion<OldScalar, TargetSpace, SourceSpace, IsRotationQuaternion> const& quaternion)
		requires (
			requires { typename std::remove_cvref_t<TargetQuaternion>::Scalar; } &&
			std::same_as<
				std::remove_cvref_t<TargetQuaternion>,
				Quaternion<typename std::remove_cvref_t<TargetQuaternion>::Scalar, TargetSpace, SourceSpace, IsRotationQuaternion>
			> &&
			requires (OldScalar const& value) { static_cast<typename std::remove_cvref_t<TargetQuaternion>::Scalar> (value); }
		)
	{
		using NewScalar = typename std::remove_cvref_t<TargetQuaternion>::Scalar;
		return narrow_cast<NewScalar> (quaternion);
	}

} // namespace neutrino::math


// Local:
#include "quaternion_operations.h"

#endif

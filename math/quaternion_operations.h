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
#include <algorithm>
#include <array>
#include <cmath>
#include <concepts>
#include <optional>
#include <type_traits>
#include <utility>


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
		return Quaternion<ResultScalar, TargetSpace, SourceSpace, false> (a.real() + b.real(), a.imag() + b.imag());
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
		return Quaternion<ResultScalar, TargetSpace, SourceSpace, false> (a.real() - b.real(), a.imag() - b.imag());
	}


template<Scalar S, class Factor, CoordinateSystem TargetSpace, CoordinateSystem SourceSpace, bool IsRotationQuaternion>
	requires ((Scalar<Factor> || std::integral<Factor>) &&
			  requires (S const& component, Factor const& factor) { { component * factor } -> Scalar; })
	[[nodiscard]]
	constexpr auto
	operator* (Quaternion<S, TargetSpace, SourceSpace, IsRotationQuaternion> const& q,
			   Factor const& scalar)
		noexcept (noexcept (q.w() * scalar))
	{
		using ResultScalar = std::remove_cvref_t<decltype (q.w() * scalar)>;
		return Quaternion<ResultScalar, TargetSpace, SourceSpace, false> (q.w() * scalar, q.x() * scalar, q.y() * scalar, q.z() * scalar);
	}


template<Scalar S, class Factor, CoordinateSystem TargetSpace, CoordinateSystem SourceSpace, bool IsRotationQuaternion>
	requires ((Scalar<Factor> || std::integral<Factor>) &&
			  requires (S const& component, Factor const& factor) { { component * factor } -> Scalar; })
	[[nodiscard]]
	constexpr auto
	operator* (Factor const& scalar,
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
		auto const inv_rotation = coordinate_system_cast<void, SourceSpace, SourceSpace, TargetSpace> (rotation.inverted());
		return coordinate_system_cast<TargetSpace, void, TargetSpace, SourceSpace> (rotation * vector_quaternion * inv_rotation).imag();
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


template<Scalar S, class Factor, CoordinateSystem TargetSpace, CoordinateSystem SourceSpace, bool IsRotationQuaternion>
	requires ((Scalar<Factor> || std::integral<Factor>) &&
			  requires (S const& component, Factor const& factor) { { component / factor } -> Scalar; })
	[[nodiscard]]
	constexpr auto
	operator/ (Quaternion<S, TargetSpace, SourceSpace, IsRotationQuaternion> const& q,
			   Factor const& scalar)
		noexcept (noexcept (q.w() / scalar))
	{
		using ResultScalar = std::remove_cvref_t<decltype (q.w() / scalar)>;
		return Quaternion<ResultScalar, TargetSpace, SourceSpace, false> (q.w() / scalar, q.x() / scalar, q.y() / scalar, q.z() / scalar);
	}


namespace detail {

/**
 * Divide finite quaternion components without constructing the divisor's inverse.
 * Keep product exponents separate until the final division, including any SI unit conversion.
 * Return nullopt for zero divisors or non-finite inputs so callers can retain their existing behavior.
 */
template<class SA, Scalar SB>
	[[nodiscard]]
	constexpr auto
	quaternion_quotient (std::array<SA, 4> const& a, std::array<SB, 4> const& b)
	{
		using ResultScalar = std::remove_cvref_t<decltype (std::declval<SA>() * (1 / std::declval<SB>()))>;
		using Value = std::common_type_t<si::decay_quantity_t<SA>, si::decay_quantity_t<SB>, si::decay_quantity_t<ResultScalar>>;
		using Result = std::optional<std::array<ResultScalar, 4>>;
		std::array<Value, 4> am {}, bm {};
		std::array<int, 4> ae {}, be {};
		Value b_max { 0 };

		for (std::size_t i = 0; i < 4; ++i)
		{
			auto const av = Value (si::value (a[i]));
			auto const bv = Value (si::value (b[i]));

			if (!std::isfinite (av) || !std::isfinite (bv))
				return Result {};

			am[i] = std::frexp (av, &ae[i]);
			bm[i] = std::frexp (bv, &be[i]);
			b_max = std::max (b_max, std::abs (bv));
		}

		if (b_max == Value (0))
			return Result {};

		int b_exponent { 0 };
		std::frexp (b_max, &b_exponent);
		Value denominator { 0 };

		for (std::size_t i = 0; i < 4; ++i)
		{
			auto const component = std::scalbn (bm[i], be[i] - b_exponent);
			denominator += component * component;
		}

		// Same-dimension quantities return a bare number, including their unit-scale ratio.
		int unit_exponent { 0 };
		auto const unit_mantissa = std::frexp (Value (si::value (SA (1) * (1 / SB (1)))), &unit_exponent);
		constexpr std::array<std::array<std::size_t, 4>, 4> indices {{
			{ 0, 1, 2, 3 }, { 1, 0, 3, 2 }, { 2, 3, 0, 1 }, { 3, 2, 1, 0 }
		}};
		constexpr std::array<std::array<int, 4>, 4> signs {{
			{ 1, 1, 1, 1 }, { -1, 1, -1, 1 }, { -1, 1, 1, -1 }, { -1, -1, 1, 1 }
		}};
		std::array<ResultScalar, 4> result;

		for (std::size_t r = 0; r < 4; ++r)
		{
			std::array<std::pair<Value, int>, 4> terms {};

			for (std::size_t c = 0; c < 4; ++c)
			{
				auto const j = indices[r][c];
				int exponent { 0 };
				auto const mantissa = std::frexp (Value (signs[r][c]) * am[c] * bm[j], &exponent);
				terms[c] = { mantissa, ae[c] + be[j] + exponent };
			}

			// Cancel large terms first so their cancellation cannot erase smaller terms.
			std::sort (terms.begin(), terms.end(), [](auto const& lhs, auto const& rhs) {
				if (lhs.first == Value (0) || rhs.first == Value (0))
					return lhs.first != Value (0);
				if (lhs.second != rhs.second)
					return lhs.second > rhs.second;
				return std::abs (lhs.first) > std::abs (rhs.first);
			});

			Value sum { 0 };
			int exponent { 0 };

			for (auto const& [mantissa, term_exponent]: terms)
			{
				if (mantissa == Value (0))
					continue;
				if (sum == Value (0))
				{
					sum = mantissa;
					exponent = term_exponent;
				}
				else
				{
					auto const common_exponent = std::max (exponent, term_exponent);
					sum = std::scalbn (sum, exponent - common_exponent) + std::scalbn (mantissa, term_exponent - common_exponent);
					int adjustment { 0 };
					sum = std::frexp (sum, &adjustment);
					exponent = common_exponent + adjustment;
				}
			}

			result[r] = ResultScalar (std::scalbn ((sum / denominator) * unit_mantissa, exponent - 2 * b_exponent + unit_exponent));
		}

		return Result (result);
	}

} // namespace detail


template<Scalar S, class Factor, CoordinateSystem TargetSpace, CoordinateSystem SourceSpace, bool IsRotationQuaternion>
	requires ((Scalar<Factor> || std::integral<Factor>) &&
			  requires (S const& component, Factor const& factor) { { factor / component } -> Scalar; })
	[[nodiscard]]
	constexpr auto
	operator/ (Factor const& scalar,
			   Quaternion<S, TargetSpace, SourceSpace, IsRotationQuaternion> const& q)
	{
		if constexpr (!(IsRotationQuaternion && std::floating_point<S>))
		{
			using Result = decltype (scalar * q.inverted());

			if (auto const components = detail::quaternion_quotient (std::array<Factor, 4> { scalar, Factor (0), Factor (0), Factor (0) }, q.components()))
				return Result (*components);
		}

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
		if constexpr (!(IsRotationQuaternionB && std::floating_point<SB>))
		{
			using Result = decltype (a * b.inverted());

			if (auto const components = detail::quaternion_quotient (a.components(), b.components()))
				return Result (*components);
		}

		return a * b.inverted();
	}


/**
 * Return the Euclidean dot product of the four quaternion components.
 */
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

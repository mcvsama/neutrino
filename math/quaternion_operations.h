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
#include <cstddef>
#include <limits>
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
	Scalar S,
	CoordinateSystem TargetSpace,
	CoordinateSystem SourceSpace,
	bool IsRotationQuaternionA,
	bool IsRotationQuaternionB
>
	[[nodiscard]]
	constexpr auto
	operator+ (Quaternion<S, TargetSpace, SourceSpace, IsRotationQuaternionA> const& a,
			   Quaternion<S, TargetSpace, SourceSpace, IsRotationQuaternionB> const& b)
		noexcept (noexcept (a.real() + b.real(), a.imag() + b.imag()))
	{
		using ResultScalar = decltype (std::declval<S>() + std::declval<S>());
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
	Scalar S,
	CoordinateSystem TargetSpace,
	CoordinateSystem SourceSpace,
	bool IsRotationQuaternionA,
	bool IsRotationQuaternionB
>
	[[nodiscard]]
	constexpr auto
	operator- (Quaternion<S, TargetSpace, SourceSpace, IsRotationQuaternionA> const& a,
			   Quaternion<S, TargetSpace, SourceSpace, IsRotationQuaternionB> const& b)
		noexcept (noexcept (a.real() - b.real(), a.imag() - b.imag()))
	{
		using ResultScalar = decltype (std::declval<S>() - std::declval<S>());
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


namespace detail {

/**
 * Sum products whose binary exponents may lie outside Value's representable range.
 * Cancel large terms first so their cancellation cannot erase smaller terms.
 */
template<std::floating_point Value, std::size_t Size>
	[[nodiscard]]
	constexpr std::pair<Value, int>
	sum_quaternion_terms (std::array<std::pair<Value, int>, Size> terms)
	{
		std::sort (terms.begin(), terms.end(), [](auto const& lhs, auto const& rhs) {
			if (lhs.first == Value (0) || rhs.first == Value (0))
				return lhs.first != Value (0);
			else if (lhs.second != rhs.second)
				return lhs.second > rhs.second;
			else
				return std::abs (lhs.first) > std::abs (rhs.first);
		});

		Value sum { 0 };
		Value correction { 0 };
		int exponent { 0 };

		for (auto const& [mantissa, term_exponent]: terms)
		{
			if (mantissa == Value (0))
				continue;

			if (sum == Value (0) && correction == Value (0))
			{
				sum = mantissa;
				exponent = term_exponent;
			}
			else
			{
				auto const common_exponent = std::max (exponent, term_exponent);
				auto const lhs = std::ldexp (sum, exponent - common_exponent);
				auto const rhs = std::ldexp (mantissa, term_exponent - common_exponent);
				sum = lhs + rhs;
				// Retain addition errors, including those exposed by later cancellation.
				correction = std::ldexp (correction, exponent - common_exponent) +
					(std::abs (lhs) >= std::abs (rhs) ? (lhs - sum) + rhs : (rhs - sum) + lhs);
				int adjustment { 0 };
				std::frexp (std::max (std::abs (sum), std::abs (correction)), &adjustment);
				sum = std::ldexp (sum, -adjustment);
				correction = std::ldexp (correction, -adjustment);
				exponent = common_exponent + adjustment;
			}
		}

		return { sum + correction, exponent };
	}


/**
 * Evaluate q * (0, v) * conjugate(q) / norm(q)^2 without forming an inverse.
 * Unit floating-point quaternions omit the denominator, as in Quaternion::inverted().
 * Use direct arithmetic for bounded inputs; keep each product's binary exponent separate
 * for extreme inputs or cancellation. Scaling whole vectors or quaternions could discard
 * small components whose contribution to the rotated vector is still representable.
 * Return nullopt for undefined rotations or non-finite inputs to preserve their old behavior.
 */
template<bool IsRotationQuaternion, Scalar SQ, Scalar SV>
	[[nodiscard]]
	constexpr auto
	quaternion_rotated_vector (std::array<SQ, 4> const& q, std::array<SV, 3> const& v)
	{
		using ResultScalar = std::remove_cvref_t<decltype ((std::declval<SQ>() * std::declval<SV>()) * (1 / std::declval<SQ>()))>;
		using ResultValue = si::decay_quantity_t<ResultScalar>;
		using Value = std::common_type_t<si::decay_quantity_t<SQ>, si::decay_quantity_t<SV>, ResultValue>;
		using Result = std::optional<std::array<ResultScalar, 3>>;
		constexpr bool unit_rotation = IsRotationQuaternion && std::floating_point<SQ>;
		// Eighth-root bounds leave room for triple products, sums, division and unit conversion.
		auto const lower = Value (std::ldexp (ResultValue (1), std::numeric_limits<ResultValue>::min_exponent / 8));
		auto const upper = Value (std::ldexp (ResultValue (1), std::numeric_limits<ResultValue>::max_exponent / 8));
		auto const within_fast_range = [=](Value magnitude) {
			return (magnitude == Value (0) || magnitude >= lower) && magnitude <= upper;
		};
		std::array<Value, 4> qv {};
		std::array<Value, 3> vv {};
		Value q_max { 0 };
		// True when every nonzero input component is safely bounded for direct arithmetic:
		bool fast_range = true;

		for (std::size_t i = 0; i < 4; ++i)
		{
			qv[i] = Value (si::value (q[i]));
			fast_range &= within_fast_range (std::abs (qv[i]));
			q_max = std::max (q_max, std::abs (qv[i]));
		}

		for (std::size_t i = 0; i < 3; ++i)
		{
			vv[i] = Value (si::value (v[i]));
			fast_range &= within_fast_range (std::abs (vv[i]));
		}

		// Preserve the scalar type and unit-scale conversions of the original Hamilton products.
		auto const unit_scale = Value (si::value ((SQ (1) * SV (1)) * (1 / SQ (1))));
		std::array<ResultScalar, 3> result;

		if (fast_range && (unit_rotation || q_max > Value (0)) && within_fast_range (std::abs (unit_scale)))
		{
			auto const w = qv[0], x = qv[1], y = qv[2], z = qv[3];
			auto const ww = w * w, xx = x * x, yy = y * y, zz = z * z;
			auto const xy = x * y, xz = x * z, yz = y * z;
			auto const wx = w * x, wy = w * y, wz = w * z;
			auto const norm_squared = ww + xx + yy + zz;
			auto const denominator = unit_rotation ? Value (1) : norm_squared;
			std::array<Value, 3> const sums {
				(ww + xx - yy - zz) * vv[0] + Value (2) * ((xy - wz) * vv[1] + (xz + wy) * vv[2]),
				Value (2) * ((xy + wz) * vv[0] + (yz - wx) * vv[2]) + (ww - xx + yy - zz) * vv[1],
				Value (2) * ((xz - wy) * vv[0] + (yz + wx) * vv[1]) + (ww - xx - yy + zz) * vv[2]
			};
			// Include cancellation inside the matrix coefficients, as well as between vector terms.
			auto const xy_wz = Value (2) * (std::abs (xy) + std::abs (wz));
			auto const xz_wy = Value (2) * (std::abs (xz) + std::abs (wy));
			auto const yz_wx = Value (2) * (std::abs (yz) + std::abs (wx));
			std::array<Value, 3> const magnitudes {
				norm_squared * std::abs (vv[0]) + xy_wz * std::abs (vv[1]) + xz_wy * std::abs (vv[2]),
				xy_wz * std::abs (vv[0]) + norm_squared * std::abs (vv[1]) + yz_wx * std::abs (vv[2]),
				xz_wy * std::abs (vv[0]) + yz_wx * std::abs (vv[1]) + norm_squared * std::abs (vv[2])
			};
			bool cancellation = false;

			for (std::size_t i = 0; i < 3; ++i)
			{
				cancellation |= std::abs (sums[i]) < Value (16) * std::numeric_limits<Value>::epsilon() * magnitudes[i];
				result[i] = ResultScalar ((sums[i] / denominator) * unit_scale);
			}

			if (!cancellation)
				return Result (result);
		}

		std::array<Value, 4> qm {};
		std::array<int, 4> qe {};
		std::array<Value, 3> vm {};
		std::array<int, 3> ve {};

		for (std::size_t i = 0; i < 4; ++i)
		{
			if (!std::isfinite (qv[i]))
				return Result {};
			qm[i] = std::frexp (qv[i], &qe[i]);
		}

		for (std::size_t i = 0; i < 3; ++i)
		{
			if (!std::isfinite (vv[i]))
				return Result {};
			vm[i] = std::frexp (vv[i], &ve[i]);
		}

		Value denominator { 1 };
		int denominator_exponent { 0 };

		if constexpr (!unit_rotation)
		{
			if (q_max == Value (0))
				return Result {};

			int q_exponent { 0 };
			std::frexp (q_max, &q_exponent);
			denominator = Value (0);
			denominator_exponent = 2 * q_exponent;

			for (std::size_t i = 0; i < 4; ++i)
			{
				auto const component = std::ldexp (qm[i], qe[i] - q_exponent);
				denominator += component * component;
			}
		}

		int unit_exponent { 0 };
		auto const unit_mantissa = std::frexp (unit_scale, &unit_exponent);
		struct Product
		{
			std::size_t q_left;
			std::size_t q_right;
			std::size_t vector_index;
			int factor;
		};

		// Each row contains eight signed q[i] * q[j] * v[k] terms; cross terms have factor two.
		constexpr std::array<std::array<Product, 8>, 3> products {{
			{{ { 0, 0, 0, 1 }, { 1, 1, 0, 1 }, { 2, 2, 0, -1 }, { 3, 3, 0, -1 },
			   { 1, 2, 1, 2 }, { 0, 3, 1, -2 }, { 1, 3, 2, 2 }, { 0, 2, 2, 2 } }},
			{{ { 0, 0, 1, 1 }, { 1, 1, 1, -1 }, { 2, 2, 1, 1 }, { 3, 3, 1, -1 },
			   { 1, 2, 0, 2 }, { 0, 3, 0, 2 }, { 2, 3, 2, 2 }, { 0, 1, 2, -2 } }},
			{{ { 0, 0, 2, 1 }, { 1, 1, 2, -1 }, { 2, 2, 2, -1 }, { 3, 3, 2, 1 },
			   { 1, 3, 0, 2 }, { 0, 2, 0, -2 }, { 2, 3, 1, 2 }, { 0, 1, 1, 2 } }}
		}};

		for (std::size_t r = 0; r < 3; ++r)
		{
			std::array<std::pair<Value, int>, 8> terms {};

			for (std::size_t c = 0; c < terms.size(); ++c)
			{
				auto const [i, j, k, factor] = products[r][c];
				int adjustment { 0 };
				auto const mantissa = std::frexp (Value (factor) * qm[i] * qm[j] * vm[k], &adjustment);
				terms[c] = { mantissa, qe[i] + qe[j] + ve[k] + adjustment };
			}

			auto const [sum, exponent] = sum_quaternion_terms (terms);
			result[r] = ResultScalar (std::ldexp ((sum / denominator) * unit_mantissa, exponent - denominator_exponent + unit_exponent));
		}

		return Result (result);
	}

} // namespace detail


template<Scalar SQ, Scalar SV, CoordinateSystem TargetSpace, CoordinateSystem SourceSpace, bool IsRotationQuaternion>
	[[nodiscard]]
	constexpr auto
	operator* (Quaternion<SQ, TargetSpace, SourceSpace, IsRotationQuaternion> const& rotation,
			   Vector<SV, 3, SourceSpace, void> const& vector)
	{
		using ResultScalar = std::remove_cvref_t<decltype ((std::declval<SQ>() * std::declval<SV>()) * (1 / std::declval<SQ>()))>;
		using Result = Vector<ResultScalar, 3, TargetSpace, void>;

		if (auto const components = detail::quaternion_rotated_vector<IsRotationQuaternion> (rotation.components(), vector.components()))
			return Result ((*components)[0], (*components)[1], (*components)[2]);

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
		// Retain the precision and units of the rotated columns when assembling the matrix.
		using ResultColumn = decltype (rotation * matrix.column (0));
		using ResultScalar = typename ResultColumn::Scalar;
		auto result = Matrix<ResultScalar, Columns, 3, TargetSpace, SourceSpace> (math::uninitialized);

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
 * Use ordinary arithmetic when all nonzero inputs and the unit conversion lie well inside the
 * result type's exponent range and no output suffers strong cancellation. Otherwise keep product
 * exponents separate until the final division, including any SI unit conversion.
 * Return nullopt for zero divisors or non-finite inputs so callers can retain their existing behavior.
 */
template<class SA, Scalar SB>
	[[nodiscard]]
	constexpr auto
	quaternion_quotient (std::array<SA, 4> const& a, std::array<SB, 4> const& b)
	{
		using ResultScalar = std::remove_cvref_t<decltype (std::declval<SA>() * (1 / std::declval<SB>()))>;
		using ResultValue = si::decay_quantity_t<ResultScalar>;
		using Value = std::common_type_t<si::decay_quantity_t<SA>, si::decay_quantity_t<SB>, si::decay_quantity_t<ResultScalar>>;
		using Result = std::optional<std::array<ResultScalar, 4>>;
		// Eighth-root bounds leave room for products, four-term sums, division and unit conversion.
		auto const lower = Value (std::ldexp (ResultValue (1), std::numeric_limits<ResultValue>::min_exponent / 8));
		auto const upper = Value (std::ldexp (ResultValue (1), std::numeric_limits<ResultValue>::max_exponent / 8));
		auto const within_fast_range = [=](Value magnitude) {
			return (magnitude == Value (0) || magnitude >= lower) && magnitude <= upper;
		};
		std::array<Value, 4> av {}, bv {};
		Value b_max { 0 };
		// True when every nonzero input component lies within the conservative bounds for direct arithmetic:
		bool fast_range = true;

		for (std::size_t i = 0; i < 4; ++i)
		{
			av[i] = Value (si::value (a[i]));
			bv[i] = Value (si::value (b[i]));
			fast_range &= within_fast_range (std::abs (av[i])) && within_fast_range (std::abs (bv[i]));
			b_max = std::max (b_max, std::abs (bv[i]));
		}

		// Same-dimension quantities return a bare number, including their unit-scale ratio.
		auto const unit_scale = Value (si::value (SA (1) * (1 / SB (1))));
		constexpr std::array<std::array<std::size_t, 4>, 4> indices {{
			{ 0, 1, 2, 3 }, { 1, 0, 3, 2 }, { 2, 3, 0, 1 }, { 3, 2, 1, 0 }
		}};
		constexpr std::array<std::array<int, 4>, 4> signs {{
			{ 1, 1, 1, 1 }, { -1, 1, -1, 1 }, { -1, 1, 1, -1 }, { -1, -1, 1, 1 }
		}};
		std::array<ResultScalar, 4> result;

		if (fast_range && b_max > Value (0) && within_fast_range (std::abs (unit_scale)))
		{
			Value denominator { 0 };

			for (auto const component: bv)
				denominator += component * component;

			bool cancellation = false;

			for (std::size_t r = 0; r < 4; ++r)
			{
				Value sum { 0 };
				Value magnitude { 0 };

				for (std::size_t c = 0; c < 4; ++c)
				{
					auto const term = Value (signs[r][c]) * av[c] * bv[indices[r][c]];
					sum += term;
					magnitude += std::abs (term);
				}

				// Recompute ill-conditioned sums with the exponent-based algorithm below.
				cancellation |= std::abs (sum) < Value (8) * std::numeric_limits<Value>::epsilon() * magnitude;
				result[r] = ResultScalar ((sum / denominator) * unit_scale);
			}

			if (!cancellation)
				return Result (result);
		}

		std::array<Value, 4> am {}, bm {};
		std::array<int, 4> ae {}, be {};

		for (std::size_t i = 0; i < 4; ++i)
		{
			if (!std::isfinite (av[i]) || !std::isfinite (bv[i]))
				return Result {};

			am[i] = std::frexp (av[i], &ae[i]);
			bm[i] = std::frexp (bv[i], &be[i]);
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

		int unit_exponent { 0 };
		auto const unit_mantissa = std::frexp (unit_scale, &unit_exponent);

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

			auto const [sum, exponent] = sum_quaternion_terms (terms);

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

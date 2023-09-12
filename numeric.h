/* vim:ts=4
 *
 * Copyleft 2012…2016  Michał Gawron
 * Marduk Unix Labs, http://mulabs.org/
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Visit http://www.gnu.org/licenses/gpl-3.0.html for more information on licensing.
 */

#ifndef NEUTRINO__NUMERIC_H__INCLUDED
#define NEUTRINO__NUMERIC_H__INCLUDED

// Neutrino:
#include <neutrino/c++20.h>
#include <neutrino/math/math.h>
#include <neutrino/range.h>
#include <neutrino/si/concepts.h>
#include <neutrino/stdexcept.h>
#include <neutrino/types.h>

// Standard:
#include <algorithm>
#include <complex>
#include <cmath>
#include <cstddef>
#include <numeric>
#include <type_traits>


namespace neutrino {

/**
 * Cast given value to a corresponding signed type.
 */
[[nodiscard]]
constexpr auto
to_signed (auto const value) noexcept
{
	return static_cast<std::make_signed_t<decltype (value)>> (value);
}


/**
 * Cast given value to a corresponding unsigned type.
 */
[[nodiscard]]
constexpr auto
to_unsigned (auto const value) noexcept
{
	return static_cast<std::make_unsigned_t<decltype (value)>> (value);
}


template<class V, class To, class From = V>
	requires (si::FloatingPointOrQuantity<From> && si::FloatingPointOrQuantity<To>)
	[[nodiscard]]
	constexpr To
	renormalize (V v, From from_min, From from_max, To to_min, To to_max)
	{
		return from_min == from_max
			? to_min
			: ((v - from_min) / (from_max - from_min)) * (to_max - to_min) + to_min;
	}


template<std::floating_point Scalar, std::floating_point Value, std::size_t Size>
	[[nodiscard]]
	constexpr auto
	renormalize (Scalar v, Scalar from_min, Scalar from_max, math::Vector<Value, Size> const& to_min, math::Vector<Value, Size> const& to_max)
	{
		math::Vector<Value, Size> result { math::zero };

		for (std::size_t i = 0; i < result.kRows; ++i)
			result[i] = renormalize (v, from_min, from_max, to_min[i], to_max[i]);

		return result;
	}


template<class V, class To, class From = V>
	[[nodiscard]]
	constexpr To
	renormalize (V value, Range<From> from_range, Range<To> to_range) noexcept
	{
		return renormalize (value, from_range.min(), from_range.max(), to_range.min(), to_range.max());
	}


/**
 * Return signum (x) (-1, 0 or 1).
 */
template<class T>
	[[nodiscard]]
	constexpr int
	sgn (T x) noexcept
	{
		if constexpr (std::is_signed_v<T>)
			return (T (0) < x) - (x < T (0));
		else
			return T (0) < x;
	}


template<class T = int>
	[[nodiscard]]
	constexpr T
	symmetric_round (auto s) noexcept
	{
		return static_cast<T> (sgn (s) * 0.5 + s);
	}


/**
 * For floats.
 * \param	n - dividend
 * \param	d - divisor
 */
template<si::FloatingPointOrQuantity Numerator, si::FloatingPointOrQuantity Denominator>
	[[nodiscard]]
	constexpr Numerator
	floored_mod (Numerator n, Denominator d)
	{
		using std::floor;
		return n - (d * floor (n / d));
	}


/**
 * For integral types.
 * \param	n - dividend
 * \param	d - divisor
 */
template<std::integral Numerator, std::integral Denominator>
	[[nodiscard]]
	constexpr Numerator
	floored_mod (Numerator n, Denominator d)
	{
		return (n % d) >= 0 ? (n % d) : (n % d) + std::abs (d);
	}


template<class Numerator, class Denominator>
	[[nodiscard]]
	constexpr Numerator
	floored_mod (Numerator n, Denominator min, Denominator max)
	{
		return floored_mod (n - min, max - min) + min;
	}


template<class Numerator, class Denominator>
	[[nodiscard]]
	constexpr Numerator
	floored_mod (Numerator n, Range<Denominator> range)
	{
		return floored_mod (n - range.min(), range.extent()) + range.min();
	}


template<class Value>
	constexpr void
	clamp (Value& value, Value min, Value max) noexcept
	{
		if (value < min)
			value = min;
		else if (value > max)
			value = max;
	}


template<class Value>
	constexpr void
	clamp (Value& value, Range<Value> range) noexcept
	{
		return range.min() <= range.max()
			? clamp (value, range.min(), range.max())
			: clamp (value, range.max(), range.min());
	}


template<class Value>
	[[nodiscard]]
	constexpr Value
	clamped (Value value, Value min, Value max) noexcept
	{
		return value < min
			? min
			: value > max
				? max
				: value;
	}


template<class Value>
	[[nodiscard]]
	constexpr Value
	clamped (Value value, Range<Value> range) noexcept
	{
		return range.min() <= range.max()
			? clamped (value, range.min(), range.max())
			: clamped (value, range.max(), range.min());
	}


template<class V>
	[[nodiscard]]
	constexpr V
	quantized (V value, std::size_t steps, Range<V> range)
	{
		Range<V> const steps_range { 0, steps };
		auto const r = std::round (renormalize (value, range, steps_range));
		auto const c = neutrino::clamped (r, steps_range);
		return renormalize (c, steps_range, range);
	}


template<class V>
	[[nodiscard]]
	constexpr V
	quantized (V value, V resolution)
	{
		return resolution * std::round (value / resolution);
	}


template<class Value>
	[[nodiscard]]
	constexpr Value
	magnetic_to_true (Value mag, Value declination)
	{
		return floored_mod (mag + declination, 360.0);
	}


[[nodiscard]]
constexpr si::Angle
magnetic_to_true (si::Angle mag, si::Angle declination)
{
	using namespace si::literals;

	return floored_mod (mag + declination, 360_deg);
}


template<class Value>
	[[nodiscard]]
	constexpr Value
	true_to_magnetic (Value tru, Value declination)
	{
		return floored_mod (tru - declination, 360.0);
	}


[[nodiscard]]
constexpr si::Angle
true_to_magnetic (si::Angle tru, si::Angle declination)
{
	using namespace si::literals;

	return floored_mod (tru - declination, 360_deg);
}


[[nodiscard]]
inline uint8_t
digit_from_ascii (char c)
{
	using namespace std::literals;

	if ('0' <= c && c <= '9')
		return c - '0';

	throw InvalidFormat ("non-numeric character '"s + c + "'");
}


/**
 * Calculate numeric integral over a range with step equal to delta.
 * Uses trapezoidal approximation.
 */
template<class Argument, class Callable>
	[[nodiscard]]
	constexpr auto
	trapezoid_integral (Callable function, Range<Argument> range, Argument delta)
	{
		using Value = std::result_of_t<Callable (Argument)>;

		auto sum = Argument() * Value();
		auto value_a = function (range.min());
		auto a = range.min();

		while (a < range.max() - delta)
		{
			auto b = a + delta;
			auto value_b = function (b);

			sum += (value_a + value_b) * delta * 0.5;
			value_a = value_b;
			a = b;
		}

		sum += (value_a + function (range.max())) * delta * 0.5;

		return sum;
	}


template<class T>
	[[nodiscard]]
	constexpr T
	static_pow (T value, uint8_t power)
	{
		T result = 1;

		for (uint8_t i = 0; i < power; ++i)
			result *= value;

		return result;
	}


template<class Iterator>
	[[nodiscard]]
	inline auto
	mean (Iterator begin, Iterator end)
	{
		if (begin == end)
			throw std::length_error ("can't compute mean() of zero-length sequence");

		using Value = std::remove_cvref_t<decltype (*std::declval<Iterator>())>;

		return std::accumulate (begin, end, Value{}) / std::distance (begin, end);
	}


/**
 * Compute median.
 * Allocates memory, but doesn't modify the sequence.
 */
template<class Iterator>
	[[nodiscard]]
	inline auto
	median (Iterator begin, Iterator end)
	{
		if (begin == end)
			throw std::length_error ("can't compute median() of zero-length sequence");

		using Value = std::remove_cvref_t<decltype (*std::declval<Iterator>())>;

		std::vector<Value> data (begin, end);
		auto const mid = data.size() / 2u;
		std::nth_element (data.begin(), data.begin() + neutrino::to_signed (mid), data.end());

		if (data.size() % 2 == 0)
			return 0.5 * (data[mid - 1] + data[mid]);
		else
			return data[mid];
	}


/**
 * Compute median allowing range to get partially sorted during operation.
 */
template<class Iterator>
	[[nodiscard]]
	inline auto
	sort_and_median (Iterator begin, Iterator end)
	{
		if (begin == end)
			throw std::length_error ("can't compute sort_and_median() of zero-length sequence");

		auto const size = std::distance (begin, end);
		auto const mid = size / 2;
		std::nth_element (begin, begin + mid, end);

		if (size % 2 == 0)
			return 0.5 * (*std::next (begin, mid - 1) + *std::next (begin, mid));
		else
			return *std::next (begin, mid);
	}


template<class Iterator>
	[[nodiscard]]
	inline auto
	stddev (Iterator begin, Iterator end)
	{
		if (begin == end)
			throw std::length_error ("can't compute stddev() of zero-length sequence");

		using std::sqrt;
		using Value = std::remove_cvref_t<decltype (*std::declval<Iterator>())>;

		std::size_t count = 0;
		auto m = mean (begin, end);
		auto sum = std::accumulate (begin, end, Value{} * Value{}, [&](auto accumulated, auto value) {
			auto const diff = value - m;
			++count;
			return accumulated + diff * diff;
		});

		return sqrt (sum / (count - 1));
	}


/**
 * Round value so that result can be used as a maximum value on a chart or histogram.
 * Returns pair of Value and a number of helper lines for chart grid.
 */
template<class Value>
	[[nodiscard]]
	inline std::pair<Value, std::size_t>
	get_max_for_axis (Value const& value)
	{
		Value fac (1);

		while (Value (1e-6) <= fac && fac <= Value (1e+6))
		{
			if (value < 0.48 * fac)
				fac /= 10.0;
			else if (value < 0.8 * fac)
				return { 1.0 * fac, 10 };
			else if (value < 1.8 * fac)
				return { 2.0 * fac, 2 };
			else if (value < 2.8 * fac)
				return { 3.0 * fac, 3 };
			else if (value < 4.8 * fac)
				return { 5.0 * fac, 5 };
			else
				fac *= 10.0;
		};

		return { fac, 10 };
	}


/**
 * Return square of the argument.
 */
template<class Value>
	[[nodiscard]]
	constexpr auto
	square (Value const a)
	{
		return a * a;
	}


/**
 * Return cube of the argument.
 */
template<class Value>
	[[nodiscard]]
	constexpr auto
	cube (Value const a)
	{
		return a * a * a;
	}


/**
 * Return array index that wraps around to always fit into range 0..size-1.
 */
[[nodiscard]]
constexpr size_t
wrap_array_index (ptrdiff_t index, size_t size)
{
	if (index >= 0)
		return to_unsigned (index) % size;
	else
	{
		auto const ssize = to_signed (size);
		return to_unsigned ((index - ssize - 1) % ssize);
	}
}


/**
 * Safe floating-point to integer rounding function that throws if rounding
 * can't be done.
 */
template<std::integral Integral, std::floating_point FloatingPoint>
	[[nodiscard]]
	inline Integral
	round_to (FloatingPoint const source)
	{
		// TODO Throw if rounding would overflow:
		return static_cast<Integral> (source);
	}

} // namespace neutrino

#endif


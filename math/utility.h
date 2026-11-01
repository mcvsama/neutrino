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

#ifndef NEUTRINO__MATH__UTILITY_H__INCLUDED
#define NEUTRINO__MATH__UTILITY_H__INCLUDED

// Standard:
#include <concepts>
#include <cstdint>
#include <limits>
#include <stdexcept>


namespace neutrino::math {

// Used to call specific constructors of Matrix or Quaternion.
class ZeroInitializer
{ };


// Used to call specific constructors of Matrix or Quaternion.
class UnitInitializer
{ };


// Used to call specific constructors of Matrix or Quaternion.
class IdentityInitializer
{ };


// Used to call specific constructors of Matrix or Quaternion.
class UninitializedInitializer
{ };


static constexpr ZeroInitializer			zero;
static constexpr UnitInitializer			unit;
static constexpr IdentityInitializer		identity;
static constexpr UninitializedInitializer	uninitialized;


/**
 * Return the Mersenne number 2^index - 1; no primality test is performed.
 * Accepts integral exponents from 0 through numeric_limits<Integer>::digits, inclusive.
 * Throws std::out_of_range for negative exponents and std::overflow_error if the result cannot fit.
 */
template<std::integral Integer, std::integral Index>
	[[nodiscard]]
	constexpr Integer
	mersenne_number (Index const index)
	{
		if constexpr (std::signed_integral<Index>)
			if (index < 0)
				throw std::out_of_range ("Mersenne exponent must be nonnegative");

		auto const exponent = static_cast<std::uintmax_t> (index);

		if (exponent > std::numeric_limits<Integer>::digits)
			throw std::overflow_error ("Mersenne number does not fit in the result type");

		// The maximum value fits even though 2^exponent does not; avoid a full-width shift.
		if (exponent == std::numeric_limits<Integer>::digits)
			return std::numeric_limits<Integer>::max();

		return static_cast<Integer> ((Integer (1) << exponent) - 1);
	}


/**
 * Compatibility alias for mersenne_number(); the result is not necessarily prime.
 */
template<std::integral Integer, std::integral Index>
	[[nodiscard]]
	constexpr Integer
	mersenne_prime (Index const index)
	{
		return mersenne_number<Integer> (index);
	}

} // namespace neutrino::math

#endif

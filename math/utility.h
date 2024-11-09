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
#include <cmath>


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


template<class Integer>
	[[nodiscard]]
	inline Integer
	mersenne_prime (auto index)
	{
		return pow (Integer (2), index) - 1;
	}

} // namespace neutrino::math

#endif


/* vim:ts=4
 *
 * Copyleft 2024  Michał Gawron
 * Marduk Unix Labs, http://mulabs.org/
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Visit http://www.gnu.org/licenses/gpl-3.0.html for more information on licensing.
 */

#ifndef NEUTRINO__MATH__CONCEPTS_H__INCLUDED
#define NEUTRINO__MATH__CONCEPTS_H__INCLUDED

// Neutrino:
#include <neutrino/si/concepts.h>

// Standard:
#include <cstddef>
#include <concepts>


namespace neutrino::math {

// Used in Matrix and Quaternion.
class CoordinateSystemBase
{ };


// Used in Matrix and Quaternion.
template<class T>
	concept CoordinateSystem = std::derived_from<T, CoordinateSystemBase> || std::same_as<void, T>;


// Used in Matrix and Quaternion.
template<class T>
	concept Scalar = neutrino::si::FloatingPointOrQuantity<T>;

} // namespace neutrino::math

#endif


/* vim:ts=4
 *
 * Copyleft 2021  Michał Gawron
 * Marduk Unix Labs, http://mulabs.org/
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Visit http://www.gnu.org/licenses/gpl-3.0.html for more information on licensing.
 */

#ifndef NEUTRINO__SI__PREDICATES_H__INCLUDED
#define NEUTRINO__SI__PREDICATES_H__INCLUDED

// Standard:
#include <cstddef>
#include <concepts>
#include <type_traits>

// Local:
#include "unit.h"


namespace neutrino::si {

// Forward
class BasicQuantity;

// Forward
class BasicUnit;


/**
 * Meta-function returning true if parameter is a Quantity type.
 * TODO use is_specialization<>
 */
template<class T>
	struct is_quantity: public std::integral_constant<bool, std::is_base_of_v<BasicQuantity, T>>
	{ };


template<class T>
	constexpr bool is_quantity_v = is_quantity<T>::value;


/**
 * Meta-function returning true if parameter is a Unit type.
 * TODO use is_specialization<>
 */
template<class T>
	struct is_unit: public std::integral_constant<bool, std::is_base_of_v<BasicUnit, T>>
	{ };


template<class T>
	constexpr bool is_unit_v = is_unit<T>::value;

} // namespace neutrino::si

#endif


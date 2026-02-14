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

#ifndef NEUTRINO__SI__CONCEPTS_H__INCLUDED
#define NEUTRINO__SI__CONCEPTS_H__INCLUDED

// Local:
#include "predicates.h"

// Standard:
#include <cstddef>
#include <concepts>


namespace neutrino::si {

template<class T>
	concept QuantityConcept = is_quantity_v<T>;


template<class T>
	concept FloatingPointOrQuantity = std::floating_point<T> || QuantityConcept<T>;


template<class Unit>
	concept UnitConcept = is_unit_v<Unit>;


template<class Value>
	concept ValueConcept = std::floating_point<Value>;


template<class Value>
	concept ScalarConcept =  std::integral<Value> || std::floating_point<Value>;


template<class Ratio>
	concept RatioConcept =
		std::integral<decltype (Ratio::num)> &&
		std::integral<decltype (Ratio::den)>;


template<class Ratio>
	concept ScaleConcept = RatioConcept<Ratio>;


template<class Ratio>
	concept OffsetConcept = RatioConcept<Ratio>;

} // namespace neutrino::si

#endif

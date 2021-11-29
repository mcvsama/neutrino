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

#ifndef NEUTRINO__CONCEPTS_H__INCLUDED
#define NEUTRINO__CONCEPTS_H__INCLUDED

// Standard:
#include <cstddef>


namespace neutrino {

template<class T>
	concept TrivialConcept = std::is_trivial_v<T>;


template<class T>
	concept ArithmeticConcept = std::integral<T> || std::floating_point<T>;

} // namespace neutrino

#endif


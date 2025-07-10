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
#include <concepts>
#include <cstddef>
#include <type_traits>


namespace neutrino {

template<class T>
	concept TrivialConcept = std::is_trivial_v<T>;


template<class T>
	concept ArithmeticConcept = std::integral<T> || std::floating_point<T>;


template<typename T, typename ...AnyOf>
	concept SameAsAnyOf = (std::same_as<T, AnyOf> || ...);


/**
 * Accepts const references, but not pr-values. Use when needing a const reference
 * to an object that outlives the function.
 * Example usage:
 *   ```
 *   void
 *   f (NonTemporaryReference<int> auto&& object);
 *
 *   int const x = 5;
 *   f (x);			// OK
 *   f (int (3));	// Error
 *   ```
 */
template<class T, class RequestedType>
	concept NonTemporaryReference =
		std::same_as<std::remove_cvref_t<T>, std::remove_cvref_t<RequestedType>> &&
		std::is_lvalue_reference_v<T>;


/**
 * Annotation to indicate that a parameter must not be a temporary object (not a pr-value).
 */
template<class T>
	using NonTemporary = T;

} // namespace neutrino

#endif


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


template<class T, class ...AnyOf>
	concept SameAsAnyOf = (std::same_as<T, AnyOf> || ...);


template<class T, class U>
	concept NotCvrefSameAs = !std::same_as<std::remove_cvref_t<T>, U>;


namespace detail {

template<class T>
	using NonTemporaryReferenceTarget = std::conditional_t<std::is_reference_v<T>, T, T const&>;

} // namespace detail


/**
 * Accepts lvalues that bind implicitly to the requested reference without creating a temporary.
 * A non-reference RequestedType means RequestedType const&; an explicit reference type is
 * preserved and must be an lvalue reference. Public, unambiguous base-class binding is allowed.
 *
 * Use with auto&& to preserve the argument's value category during deduction. The concept
 * checks reference binding; it does not make the deduced parameter const or guarantee that
 * the referenced object outlives subsequent uses of the reference.
 *
 * Example usage:
 *   ```
 *   void
 *   f (NonTemporaryReference<int> auto&& object);
 *
 *   int const x = 5;
 *   f (x);			// OK
 *   f (int (3));	// Error: rvalue
 *   double y = 3;
 *   f (y);			// Error: conversion would create a temporary int
 *   ```
 */
template<class T, class RequestedType>
	concept NonTemporaryReference =
		std::is_lvalue_reference_v<T> &&
		std::is_lvalue_reference_v<detail::NonTemporaryReferenceTarget<RequestedType>> &&
		std::is_convertible_v<T, detail::NonTemporaryReferenceTarget<RequestedType>> &&
		!std::reference_converts_from_temporary_v<detail::NonTemporaryReferenceTarget<RequestedType>, T>;


/**
 * Annotation to indicate that a parameter must not be a temporary object (not a pr-value).
 * This alias is documentation-only and does not enforce the restriction.
 */
template<class T>
	using NonTemporary = T;

} // namespace neutrino

#endif

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

#ifndef NEUTRINO__MATH__TRAITS_H__INCLUDED
#define NEUTRINO__MATH__TRAITS_H__INCLUDED

// Standard:
#include <cstddef>
#include <type_traits>


namespace neutrino::math {

template<class pValue>
	struct Traits
	{
		typedef pValue Value;

		[[nodiscard]]
		static Value
		zero();

		[[nodiscard]]
		static Value
		one();

		[[nodiscard]]
		static auto
		inverted (Value const&);
	};


template<class Value>
	inline Value
	Traits<Value>::zero()
	{
		return 0;
	}


template<class Value>
	inline Value
	Traits<Value>::one()
	{
		return 1;
	}


template<class Value>
	inline auto
	Traits<Value>::inverted (Value const& v)
	{
		return one() / v;
	}


/*
 * General functions
 */


/**
 * Return the inverse through Traits; its type may differ from Value.
 */
template<class Value>
	[[nodiscard]]
	auto
	inverted (Value const& value)
	{
		return Traits<Value>::inverted (value);
	}

} // namespace neutrino::math

#endif

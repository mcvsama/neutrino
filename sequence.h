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

#ifndef NEUTRINO__SEQUENCE_H__INCLUDED
#define NEUTRINO__SEQUENCE_H__INCLUDED

// Standard:
#include <algorithm>
#include <cstddef>
#include <iterator>
#include <utility>


namespace neutrino {

template<class pIterator>
	class Sequence: public std::iterator_traits<pIterator>
	{
	  public:
		using Iterator = pIterator;

	  public:
		// Ctor
		constexpr
		Sequence (Iterator begin, Iterator end);

		constexpr Iterator
		begin() const noexcept
			{ return _begin; }

		constexpr Iterator
		end() const noexcept
			{ return _end; }

		constexpr size_t
		empty() const
			{ return std::distance (_begin, _end) == 0; }

		constexpr size_t
		size() const
			{ return std::distance (_begin, _end); }

	  private:
		Iterator const	_begin;
		Iterator const	_end;
	};


template<class I>
	constexpr
	Sequence<I>::Sequence (Iterator begin, Iterator end):
		_begin (begin),
		_end (end)
	{ }

} // namespace neutrino

#endif


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

#ifndef NEUTRINO__MEMORY_H__INCLUDED
#define NEUTRINO__MEMORY_H__INCLUDED

// Standard:
#include <cstddef>
#include <cstring>


namespace neutrino {

template<class Value, class OutputIterator>
	inline OutputIterator
	copy_value_to_memory (Value const& value, OutputIterator const target)
	{
		std::memcpy (&*target, &value, sizeof (Value));
		return target + sizeof (Value);
	}


template<class Value, class InputIterator>
	inline InputIterator
	copy_memory_to_value (InputIterator const source, Value& value)
	{
		std::memcpy (&value, &*source, sizeof (Value));
		return source + sizeof (Value);
	}

} // namespace neutrino

#endif

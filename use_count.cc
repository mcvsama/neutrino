/* vim:ts=4
 *
 * Copyleft 2025  Michał Gawron
 * Marduk Unix Labs, http://mulabs.org/
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Visit http://www.gnu.org/licenses/gpl-3.0.html for more information on licensing.
 */

// Local:
#include "use_count.h"

// Standard:
#include <cstddef>


namespace neutrino {

UseCount::~UseCount()
{
	if (_counter > 0)
		Exception::terminate ("Error: deleting " + _object_type + " on which other objects still depend (UseCount > 0).");
}


UseCount&
UseCount::operator--()
{
	if (_counter == 0)
		throw InvalidCall ("Can't decrement UseCount, it's already 0");
	else
		--_counter;

	return *this;
}


UseToken&
UseToken::operator= (UseToken const& other)
{
	--*_use_count;
	_use_count = other._use_count;
	++*_use_count;
	return *this;
}

} // namespace neutrino


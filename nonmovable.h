/* vim:ts=4
 *
 * Copyleft 2008…2013  Michał Gawron
 * Marduk Unix Labs, http://mulabs.org/
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Visit http://www.gnu.org/licenses/gpl-3.0.html for more information on licensing.
 */

#ifndef NEUTRINO__NONMOVABLE_H__INCLUDED
#define NEUTRINO__NONMOVABLE_H__INCLUDED


namespace neutrino {

class Nonmovable
{
	Nonmovable (Nonmovable const&) = delete;
	Nonmovable& operator= (Nonmovable const&) = delete;

  protected:
	constexpr Nonmovable() noexcept
	{ }
};

} // namespace neutrino

#endif


/* vim:ts=4
 *
 * Copyleft 2012…2019  Michał Gawron
 * Marduk Unix Labs, http://mulabs.org/
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Visit http://www.gnu.org/licenses/gpl-3.0.html for more information on licensing.
 */

#ifndef NEUTRINO__CORE__VERSION_H__INCLUDED
#define NEUTRINO__CORE__VERSION_H__INCLUDED

namespace neutrino::version {

/**
 * References to dynamically created commit ID and branch name.
 * File version.cc will be made and compiled by build system.
 */
extern const char* commit;
extern const char* branch;
extern const char* version;

} // namespace neutrino::version

#endif


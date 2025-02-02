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

#ifndef NEUTRINO__QT__QSTRING_H__INCLUDED
#define NEUTRINO__QT__QSTRING_H__INCLUDED

// Qt:
#include <QString>

// Standard:
#include <cstddef>
#include <string_view>


namespace neutrino {

QString
to_qstring (std::u8string_view const sv)
{
	// FIXME Qt 6.1 is supposed to support fromUtf8() with char8_t, but… it doesn't compile.
	return QString::fromUtf8 (reinterpret_cast<const char*> (sv.data()), static_cast<qsizetype> (sv.size()));
}

} // namespace neutrino

#endif


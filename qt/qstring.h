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
#include <string>
#include <string_view>


namespace neutrino {

inline QString
to_qstring (std::string const& str)
{
	return QString::fromStdString (str);
}


inline QString
to_qstring (std::u8string const& str)
{
	return QString::fromUtf8 (str.data(), static_cast<qsizetype> (str.size())); // TODO use gsl::narrow<qsizetype> (and repeat for other overloads)
}


inline QString
to_qstring (std::u8string_view const sv)
{
	return QString::fromUtf8 (sv.data(), static_cast<qsizetype> (sv.size()));
}


inline QString
to_qstring (std::u16string const& str)
{
	return QString::fromStdU16String (str);
}


inline QString
to_qstring (std::u16string_view const sv)
{
	return QString::fromUtf16 (sv.data(), static_cast<qsizetype> (sv.size()));
}


inline QString
to_qstring (std::u32string const& str)
{
	return QString::fromStdU32String (str);
}


inline QString
to_qstring (std::u32string_view const sv)
{
	return QString::fromUcs4 (sv.data(), static_cast<qsizetype> (sv.size()));
}


inline void
filter_printable_string (QString& input, QChar const replacement = '.')
{
	for (int i = 0; i < input.size(); ++i)
		if (!input[i].isPrint() || input[i] == u'�')
			input[i] = replacement;
}


inline QString
filter_printable_string (QStringView const input, QChar const replacement = '.')
{
	QString s (input);
	filter_printable_string (input, replacement);
	return s;
}

} // namespace neutrino

#endif


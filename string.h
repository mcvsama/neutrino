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

#ifndef NEUTRINO__STRING_H__INCLUDED
#define NEUTRINO__STRING_H__INCLUDED

// Standard:
#include <cstddef>
#include <string>
#include <string_view>

// Qt:
#include <QString>

// Neutrino:
#include <neutrino/blob.h>


namespace neutrino {

inline std::string
to_hex_string (std::string_view const blob, std::string_view const separator = "")
{
	if (blob.empty())
		return "";

	std::string s;

	for (size_t i = 0; i < blob.size(); ++i)
	{
		auto const v = blob[i];
		s += QString ("%1").arg (static_cast<uint8_t> (v), 2, 16, QChar ('0')).toStdString();

		if (i < blob.size() - 1)
			s += separator;
	}

	return s;
}


inline std::string
to_hex_string (BlobView const blob, std::string_view const separator = "")
{
	return to_hex_string (std::string (blob.cbegin(), blob.cend()), separator);
}

} // namespace neutrino

#endif

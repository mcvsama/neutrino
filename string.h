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

// Neutrino:
#include <neutrino/blob.h>

// Standard:
#include <cstddef>
#include <string>
#include <string_view>


namespace neutrino {

inline std::string
to_string (BlobView const blob)
{
	return { blob.cbegin(), blob.cend() };
}


inline std::string
to_hex_string (std::string_view const blob, std::string_view const separator = "")
{
	static constexpr char hextable[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };

	if (blob.empty())
		return "";

	std::string s;

	for (size_t i = 0; i < blob.size(); ++i)
	{
		auto const v = blob[i];
		s += hextable[(v >> 4) & 0xf];
		s += hextable[(v >> 0) & 0xf];

		if (i < blob.size() - 1)
			s += separator;
	}

	return s;
}


inline std::string
to_hex_string (BlobView const blob, std::string_view const separator = "")
{
	return to_hex_string (to_string (blob), separator);
}


inline std::string
to_printable_string (std::string_view const blob)
{
	std::string s;

	for (size_t i = 0u; i < blob.size(); ++i)
	{
		auto const c = blob[i];
		auto const u = static_cast<unsigned char> (c);

		// std::isprint() is non-UB only if it's argument is converted to unsigned char first.
		// I know, I know…
		if (std::isprint (u))
			s.push_back (c);
		else
			s.append (std::format ("\\{:#02x}", u));
	}

	return s;
}

} // namespace neutrino

#endif


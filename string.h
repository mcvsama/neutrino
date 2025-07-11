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
#include <neutrino/demangle.h>
#include <neutrino/stdexcept.h>

// Standard:
#include <cstddef>
#include <format>
#include <string>
#include <string_view>


namespace neutrino {

inline std::string
to_string (BlobView const blob)
{
	return { blob.cbegin(), blob.cend() };
}


std::string
to_hex_string (std::string_view const blob, std::string_view const separator = "");


inline std::string
to_hex_string (BlobView const blob, std::string_view const separator = "")
{
	return to_hex_string (to_string (blob), separator);
}


std::string
to_printable_string (std::string_view const blob);


/**
 * Replace non-printable characters with a chosen character.
 */
void
filter_printable_string (std::string& input, char replacement = '.');


inline std::string
filter_printable_string (std::string_view const input, char replacement = '.')
{
	std::string s (input);
	filter_printable_string (s, replacement);
	return s;
}


template<std::integral Value, class Char>
	[[nodiscard]]
	inline Value
	parse (std::basic_string_view<Char> const& str, int base = 10)
	{
		Value converted;
		auto [ptr, conv_error] = std::from_chars (str.begin(), str.end(), converted, base);

		if (ptr != str.end() || conv_error != std::errc())
			throw ParseException (std::format ("failed to parse '{}' as {}", str, demangle (typeid (Value).name())));
		else
			return converted;
	}


template<std::integral Value, class Char>
	[[nodiscard]]
	inline Value
	parse (std::basic_string<Char> const& str, int base = 10)
	{
		std::basic_string_view<Char> sv = str;
		return parse<Value> (sv, base);
	}


template<std::floating_point Value, class Char>
	[[nodiscard]]
	inline Value
	parse (std::basic_string_view<Char> const& str, std::chars_format const format = std::chars_format::general)
	{
		Value converted;
		auto [ptr, conv_error] = std::from_chars (str.begin(), str.end(), converted, format);

		if (ptr != str.end() || conv_error != std::errc())
			throw ParseException (std::format ("failed to parse '{}' as {}", str, demangle (typeid (Value).name())));
		else
			return converted;
	}


template<std::floating_point Value, class Char>
	[[nodiscard]]
	inline Value
	parse (std::basic_string<Char> const& str, std::chars_format const format = std::chars_format::general)
	{
		std::basic_string_view<Char> sv = str;
		return parse<Value> (sv, format);
	}


template<std::floating_point Value, class Char>
	[[nodiscard]]
	inline Value
	parse (const Char* str, std::chars_format const format = std::chars_format::general)
	{
		std::basic_string_view<Char> sv = str;
		return parse<Value> (sv, format);
	}

} // namespace neutrino

#endif


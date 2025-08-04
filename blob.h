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

#ifndef NEUTRINO__BLOB_H__INCLUDED
#define NEUTRINO__BLOB_H__INCLUDED

// Neutrino:
#include <neutrino/concepts.h>
#include <neutrino/core_types.h>
#include <neutrino/endian.h>
#include <neutrino/stdexcept.h>

// Standard:
#include <cstddef>
#include <cstring>
#include <format>
#include <string>
#include <string_view>
#include <type_traits>


namespace neutrino {

using Blob = std::basic_string<uint8_t, std::char_traits<uint8_t>, boost::alignment::aligned_allocator<uint8_t, 32>>;
using BlobView = std::basic_string_view<uint8_t>;


template<class T>
	concept BlobSerializableValueConcept = std::integral<T> || std::floating_point<T> || std::is_enum_v<T>;


/**
 * Thrown by unserialize() functions.
 */
class InvalidBlobSize: public InvalidArgument
{
  public:
	explicit InvalidBlobSize (size_t is, std::optional<size_t> should_be = {}):
		InvalidArgument (std::format ("invalid blob size {}{}", is, (should_be ? ", should be " + std::to_string (*should_be) : "")))
	{ }
};


[[nodiscard]]
inline Blob
to_blob (bool value)
{
	return Blob (1, value);
}


[[nodiscard]]
inline Blob
to_blob (std::string_view const str)
{
	return Blob (str.cbegin(), str.cend());
}


/**
 * Explicit overload for C-strings is needed, otherwise bool-version would be chosen
 * over std::string_view one, as it doesn't need implicing conversion to std::string_view.
 */
[[nodiscard]]
inline Blob
to_blob (char const* str)
{
	return to_blob (std::string_view (str));
}


[[nodiscard]]
inline Blob
to_blob (BlobSerializableValueConcept auto const value)
{
	Blob blob;
	blob.resize (sizeof (value));
	neutrino::perhaps_native_to_little_inplace (value);
	std::memcpy (blob.data(), &value, sizeof (value));
	return blob;
}


template<std::same_as<std::string> Value>
	[[nodiscard]]
	inline Value
	parse (BlobView const blob)
	{
		return std::string (blob.cbegin(), blob.cend());
	}


template<BlobSerializableValueConcept Value>
	[[nodiscard]]
	inline Value
	parse (BlobView const blob)
	{
		if constexpr (std::is_same<Value, bool>())
		{
			if (blob.size() != 1)
				throw InvalidBlobSize (blob.size(), 1);

			return !!blob[0];
		}
		else
		{
			Value value;

			if (blob.size() != sizeof (value))
				throw InvalidBlobSize (blob.size(), sizeof (value));

			std::memcpy (&value, blob.data(), sizeof (value));
			neutrino::perhaps_little_to_native_inplace (value);

			return value;
		}
	}


/**
 * Wipe argument's buffer with unspecified data.
 */
inline void
wipe (Blob& blob)
{
	for (auto& c: blob)
		c = 0xaa;
}

} // namespace neutrino

#endif


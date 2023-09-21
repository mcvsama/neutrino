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
#include <neutrino/si/si.h>
#include <neutrino/stdexcept.h>

// Standard:
#include <cstddef>
#include <cstring>
#include <string>
#include <type_traits>


namespace neutrino {

template<class T>
	concept BlobSerializableValueConcept = std::integral<T> || std::floating_point<T> || std::is_enum_v<T>;


/**
 * Thrown by unserialize() functions.
 */
class InvalidBlobSize: public InvalidArgument
{
  public:
	explicit InvalidBlobSize (size_t is, std::optional<size_t> should_be = {}):
		InvalidArgument ("invalid blob size " + std::to_string (is) +
						 (should_be ? ", should be " + std::to_string (*should_be) : ""))
	{ }
};


inline Blob
to_blob (std::string_view const str)
{
	return Blob (str.begin(), str.end());
}


inline void
value_to_blob (bool value, Blob& blob)
{
	blob.resize (1);
	blob[0] = !!value;
}


inline void
value_to_blob (std::string_view const str, Blob& blob)
{
	blob.assign (str.cbegin(), str.cend());
}


/**
 * Explicit overload for C-strings is needed, otherwise bool-version would be chosen
 * over std::strin_view one, as it doesn't need implicing conversion to std::string_view.
 */
inline void
value_to_blob (char const* str, Blob& blob)
{
	value_to_blob (std::string_view (str), blob);
}


template<BlobSerializableValueConcept Value>
	inline void
	value_to_blob (Value const value, Blob& blob)
	{
		blob.resize (sizeof (value));
		neutrino::perhaps_native_to_little_inplace (value);
		std::memcpy (blob.data(), &value, sizeof (value));
	}


inline void
value_to_blob (si::QuantityConcept auto value, Blob& blob)
{
	blob = si::to_blob (value);
}


template<class Value>
	[[nodiscard]]
	inline Blob
	value_to_blob (Value&& value)
	{
		Blob blob;
		value_to_blob (std::forward<Value> (value), blob);
		return blob;
	}


inline void
blob_to_value (BlobView const blob, bool& value)
{
	if (blob.size() != 1)
		throw InvalidBlobSize (blob.size(), 1);

	value = !!blob[0];
}


inline void
blob_to_value (BlobView const blob, std::string& value)
{
	value.assign (blob.cbegin(), blob.cend());
}


template<BlobSerializableValueConcept Value>
	inline void
	blob_to_value (BlobView const blob, Value& value)
	{
		if (blob.size() != sizeof (value))
			throw InvalidBlobSize (blob.size(), sizeof (value));

		std::memcpy (&value, blob.data(), sizeof (value));
		neutrino::perhaps_little_to_native_inplace (value);
	}


inline void
blob_to_value (BlobView const blob, si::QuantityConcept auto& value)
{
	// TODO Inefficient, change to use BlobView.
	si::parse (Blob { blob.cbegin(), blob.cend() }, value);
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


/* vim:ts=4
 *
 * Copyleft 2021  Michał Gawron
 * Marduk Unix Labs, http://mulabs.org/
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Visit http://www.gnu.org/licenses/gpl-3.0.html for more information on licensing.
 */

#ifndef NEUTRINO__CRYPTO__HKDF_H__INCLUDED
#define NEUTRINO__CRYPTO__HKDF_H__INCLUDED

// Neutrino:
#include <neutrino/blob.h>
#include <neutrino/crypto/hmac.h>
#include <neutrino/crypto/hash.h>
#include <neutrino/stdexcept.h>

// Standard:
#include <algorithm>
#include <cstddef>


namespace neutrino {

struct HKDF_Args
{
	BlobView	salt;
	BlobView	key_material;
	BlobView	info;
	size_t		result_length;
};


/**
 * Derive up to 255 hash-result blocks, as required by RFC 5869 section 2.3.
 * Throw InvalidArgument if result_length exceeds this limit; zero is allowed.
 */
template<Hash::Algorithm Algorithm>
	[[nodiscard]]
	inline Blob
	compute_hkdf (HKDF_Args const& args)
	{
		using Transform = typename decltype (get_hash_function<Algorithm>())::Transform;

		constexpr auto digest_size = std::size_t (Transform::DIGESTSIZE);
		// HKDF-Expand encodes its block counter in a single octet (1 through 255).
		constexpr auto max_result_length = 255 * digest_size;

		if (args.result_length > max_result_length)
			throw InvalidArgument ("HKDF output length exceeds 255 hash blocks");

		if (args.result_length == 0)
			return {};

		// All intermediate secrets are wiped on normal return and exception unwinding.
		auto prk = CryptoPP::SecByteBlock (digest_size);
		auto extract = CryptoPP::HMAC<Transform> (args.salt.data(), args.salt.size());
		extract.CalculateDigest (prk.data(), args.key_material.data(), args.key_material.size());

		auto expand = CryptoPP::HMAC<Transform> (prk.data(), prk.size());
		auto next_key = CryptoPP::SecByteBlock (digest_size);
		auto result = CryptoPP::SecByteBlock (args.result_length);
		size_t offset = 0;

		for (size_t i = 1; offset < args.result_length; ++i)
		{
			if (offset != 0)
				expand.Update (next_key.data(), next_key.size());

			expand.Update (args.info.data(), args.info.size());
			auto const counter = static_cast<CryptoPP::byte> (i);
			expand.Update (&counter, 1);
			expand.Final (next_key.data());

			auto const count = std::min (digest_size, args.result_length - offset);
			std::copy_n (next_key.data(), count, result.data() + offset);
			offset += count;
		}

		return Blob (result.data(), result.size());
	}

} // namespace neutrino

#endif

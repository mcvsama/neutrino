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
		// HKDF-Expand encodes its block counter in a single octet (1 through 255).
		auto const max_result_length = 255 * get_hash_function<Algorithm>().result_size();

		if (args.result_length > max_result_length)
			throw InvalidArgument ("HKDF output length exceeds 255 hash blocks");

		Blob const prk = compute_hmac<Algorithm> ({
			.data = args.key_material,
			.key = args.salt,
		});
		Blob const info (args.info);
		Blob next_key;
		Blob result;
		size_t i = 1;

		result.reserve (args.result_length);

		while (result.size() < args.result_length)
		{
			next_key = compute_hmac<Algorithm> ({
				.data = next_key + info + Blob (1, static_cast<Blob::value_type> (i)),
				.key = prk,
			});
			result += next_key;
			++i;
		}

		return result.substr (0, args.result_length);
	}

} // namespace neutrino

#endif

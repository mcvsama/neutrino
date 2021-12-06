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

// Standard:
#include <cstddef>

// Neutrino:
#include <neutrino/blob.h>
#include <neutrino/crypto/hmac.h>
#include <neutrino/crypto/hash.h>


namespace neutrino {

struct HKDF_Args
{
	BlobView	salt;
	BlobView	key_material;
	BlobView	info;
	size_t		result_length;
};


template<Hash::Algorithm Algorithm>
	inline Blob
	calculate_hkdf (HKDF_Args const& args)
	{
		Blob const prk = calculate_hmac<Algorithm> ({
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
			next_key = calculate_hmac<Algorithm> ({
				.data = next_key + info + Blob (1, i),
				.key = prk,
			});
			result += next_key;
			++i;
		}

		return result.substr (0, args.result_length);
	}

} // namespace neutrino

#endif


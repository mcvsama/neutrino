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

#ifndef NEUTRINO__CRYPTO__HMAC_H__INCLUDED
#define NEUTRINO__CRYPTO__HMAC_H__INCLUDED

// Standard:
#include <cstddef>

// Neutrino:
#include <neutrino/crypto/hash.h>
#include <neutrino/strong_type.h>


namespace neutrino {

/**
 * HMAC function parameters struct.
 */
struct HMAC_Args
{
	BlobView	data;
	BlobView	key;
};


template<Hash::Algorithm Algorithm>
	inline Blob
	calculate_hmac (HMAC_Args const& args)
	{
		auto hash = get_hash_function<Algorithm>();
		auto const block_size = hash.block_size();
		Blob key (args.key);

		if (key.size() > block_size)
			key = hash (key);

		if (key.size() < block_size)
			key.resize (block_size, 0);

		Blob opad (block_size, 0x5c);
		Blob ipad (block_size, 0x36);

		for (size_t i = 0; i < block_size; ++i)
			opad[i] ^= key[i];

		for (size_t i = 0; i < block_size; ++i)
			ipad[i] ^= key[i];

		return hash (opad + hash (ipad + Blob (args.data)));
	}

} // namespace neutrino

#endif


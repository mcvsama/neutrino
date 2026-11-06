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

// Neutrino:
#include <neutrino/crypto/hash.h>

// Crypto++:
#include <cryptopp/hmac.h>
#include <cryptopp/secblock.h>

// Standard:
#include <cstddef>


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
	[[nodiscard]]
	inline Blob
	compute_hmac (HMAC_Args const& args)
	{
		using Transform = typename decltype (get_hash_function<Algorithm>())::Transform;

		// Crypto++ keeps the normalized key, pads and inner digest in wiping buffers.
		auto hmac = CryptoPP::HMAC<Transform> (args.key.data(), args.key.size());
		auto result = CryptoPP::SecByteBlock (hmac.DigestSize());
		hmac.CalculateDigest (result.data(), args.data.data(), args.data.size());
		return Blob (result.data(), result.size());
	}

} // namespace neutrino

#endif

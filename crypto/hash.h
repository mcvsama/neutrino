/* vim:ts=4
 *
 * Copyleft 2012…2017  Michał Gawron
 * Marduk Unix Labs, http://mulabs.org/
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Visit http://www.gnu.org/licenses/gpl-3.0.html for more information on licensing.
 */

#ifndef NEUTRINO__CRYPTO__HASH_H__INCLUDED
#define NEUTRINO__CRYPTO__HASH_H__INCLUDED

// Neutrino:
#include <neutrino/blob.h>
#include <neutrino/crypto/hash_interface.h>
#include <neutrino/crypto/hash_cryptopp.h>

// Standard:
#include <stdexcept>


namespace neutrino {

template<Hash::Algorithm Algorithm>
	[[nodiscard]]
	constexpr auto
	get_hash_function()
	{
		if constexpr (Algorithm == Hash::SHA2_256)
			return HashSHA2_256();
		else if constexpr (Algorithm == Hash::SHA3_224)
			return HashSHA3_224();
		else if constexpr (Algorithm == Hash::SHA3_256)
			return HashSHA3_256();
		else if constexpr (Algorithm == Hash::SHA3_384)
			return HashSHA3_384();
		else if constexpr (Algorithm == Hash::SHA3_512)
			return HashSHA3_512();
		else
			throw std::logic_error ("Unhandled algorithm - please update this function");
	}


template<Hash::Algorithm Algorithm>
	[[nodiscard]]
	inline Blob
	compute_hash (BlobView const data)
	{
		auto hasher = get_hash_function<Algorithm>();
		hasher.update (data);
		return Blob (hasher.result());
	}

} // namespace neutrino

#endif

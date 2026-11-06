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

#ifndef NEUTRINO__CRYPTO__AES_H__INCLUDED
#define NEUTRINO__CRYPTO__AES_H__INCLUDED

// Neutrino:
#include <neutrino/blob.h>
#include <neutrino/exception.h>

// Boost:
#include <boost/endian/conversion.hpp>

// Lib:
#include <cryptopp/aes.h>
#include <cryptopp/secblock.h>
#include <cryptopp/simple.h>

// Standard:
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>


namespace neutrino {

struct AES_Args
{
	// Data to encrypt/decrypt:
	BlobView	data;

	// Key length must be 16, 24 or 32 bytes (AES-128, AES-192 or AES-256).
	BlobView	key;
};


struct CTR_128_Args
{
	// Data to encrypt/decrypt:
	BlobView	data;

	// Key length must be 16, 24 or 32 bytes (AES-128, AES-192 or AES-256).
	BlobView	key;

	// Unique 8-byte nonce for each data packet (alternatively nonce can be const, but
	// key must be different each time):
	BlobView	nonce;
};


[[nodiscard]]
inline Blob
aes_encrypt (AES_Args const& args)
{
	if (args.data.size() != CryptoPP::AES::BLOCKSIZE)
		throw std::runtime_error ("invalid data block size " + std::to_string (args.data.size()) + " B, must be " + std::to_string (CryptoPP::AES::BLOCKSIZE) + " B");

	if (args.key.size() < CryptoPP::AES::MIN_KEYLENGTH || args.key.size() > CryptoPP::AES::MAX_KEYLENGTH)
		throw std::runtime_error ("invalid key block size " + std::to_string (args.key.size()) + " B, must be [" +
								  std::to_string (CryptoPP::AES::MIN_KEYLENGTH) + ".." + std::to_string (CryptoPP::AES::MAX_KEYLENGTH) + "] B");

	auto result = CryptoPP::SecByteBlock (CryptoPP::AES::BLOCKSIZE);

	CryptoPP::AESEncryption enc;
	enc.SetKey (args.key.data(), args.key.size());
	enc.ProcessBlock (args.data.data(), result.data());

	return Blob (result.data(), result.size());
}


[[nodiscard]]
inline Blob
aes_ctr_xor (CTR_128_Args const& args)
{
	// Validate lengths before the empty-input fast path, without expanding the key.
	constexpr auto nonce_size = CryptoPP::AES::BLOCKSIZE - sizeof (uint64_t);

	if (args.nonce.size() != nonce_size)
		throw std::runtime_error ("invalid AES-CTR nonce size");

	if (CryptoPP::AES::StaticGetValidKeyLength (args.key.size()) != args.key.size())
		throw CryptoPP::InvalidKeyLength (CryptoPP::AES::StaticAlgorithmName(), args.key.size());

	if (args.data.empty())
		return {};

	CryptoPP::AESEncryption enc;
	enc.SetKey (args.key.data(), args.key.size());

	// Keep only one keystream block, and wipe it and the working output on every exit.
	auto block = CryptoPP::SecByteBlock (CryptoPP::AES::BLOCKSIZE);
	auto key_stream = CryptoPP::SecByteBlock (CryptoPP::AES::BLOCKSIZE);
	auto result = CryptoPP::SecByteBlock (args.data.size());
	std::copy (args.nonce.begin(), args.nonce.end(), block.begin());
	size_t offset = 0;

	for (uint64_t i = 0; offset < result.size(); ++i)
	{
		auto const big_i = boost::endian::native_to_big (i);
		std::memcpy (block.data() + nonce_size, &big_i, sizeof (big_i));
		enc.ProcessBlock (block.data(), key_stream.data());

		auto const count = std::min (key_stream.size(), result.size() - offset);

		for (size_t j = 0; j < count; ++j)
			result[offset + j] = args.data[offset + j] ^ key_stream[j];

		offset += count;
	}

	return Blob (result.data(), result.size());
}

} // namespace neutrino

#endif

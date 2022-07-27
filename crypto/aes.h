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

// Standard:
#include <cstddef>


namespace neutrino {

struct AES_Args
{
	BlobView	data;
	BlobView	key;
};


struct CTR_128_Args
{
	BlobView	data;
	BlobView	key;
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

	Blob result;
	result.resize (CryptoPP::AES::BLOCKSIZE);

	CryptoPP::AESEncryption enc;
	enc.SetKey (args.key.data(), args.key.size());
	enc.ProcessBlock (args.data.data(), result.data());

	return result;
}


[[nodiscard]]
inline Blob
aes_ctr_xor (CTR_128_Args const& args)
{
	Blob result = Blob (args.data);
	Blob key_stream;

	// Key stream needs to be at least the size of input data:
	for (uint64_t i = 0; key_stream.size() < result.size(); ++i)
	{
		uint64_t big_i = boost::endian::native_to_big (i);
		Blob counter (sizeof (big_i), 0x00);
		std::memcpy (counter.data(), &big_i, sizeof (big_i));
		key_stream += aes_encrypt ({ .data = Blob (args.nonce) + counter, .key = args.key });
	}

	for (size_t i = 0; i < result.size(); ++i)
		result[i] ^= key_stream[i];

	return result;
}

} // namespace neutrino

#endif


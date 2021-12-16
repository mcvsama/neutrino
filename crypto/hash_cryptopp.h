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

#ifndef NEUTRINO__CRYPTO__HASH_CRYPTOPP_H__INCLUDED
#define NEUTRINO__CRYPTO__HASH_CRYPTOPP_H__INCLUDED

// Standard:
#include <cstddef>

// Lib:
#include <cryptopp/sha.h>
#include <cryptopp/sha3.h>

// Neutrino:
#include <neutrino/blob.h>
#include <neutrino/crypto/hash.h>


namespace neutrino {

// Forward:
template<class HashTransformType>
	class CryptoPPHash;


using HashSHA2_256 = CryptoPPHash<CryptoPP::SHA256>;
using HashSHA3_224 = CryptoPPHash<CryptoPP::SHA3_224>;
using HashSHA3_256 = CryptoPPHash<CryptoPP::SHA3_256>;
using HashSHA3_384 = CryptoPPHash<CryptoPP::SHA3_384>;
using HashSHA3_512 = CryptoPPHash<CryptoPP::SHA3_512>;


template<class HashTransformType>
	class CryptoPPHash: public Hash
	{
	  public:
		// Hash API
		void
		update (BlobView const blob) override;

		// Hash API
		[[nodiscard]]
		Blob
		result() override;

		// Hash API
		[[nodiscard]]
		BlobView
		view_result() override;

		// Hash API
		[[nodiscard]]
		bool
		finalized() const override
			{ return !_result.empty(); }

		// Hash API
		[[nodiscard]]
		size_t
		block_size() const override
			{ return _hash.BlockSize(); }

		// Hash API
		[[nodiscard]]
		size_t
		result_size() const override
			{ return _hash.DigestSize(); }

		// Hash API
		void
		reset() override;

	  private:
		HashTransformType	_hash;
		Blob				_result;
	};


template<class H>
	inline void
	CryptoPPHash<H>::update (BlobView const blob)
	{
		if (finalized())
			throw AlreadyFinalized();

		_hash.Update (blob.data(), blob.size());
	}


template<class H>
	inline Blob
	CryptoPPHash<H>::result()
	{
		if (_result.empty())
		{
			_result.resize (result_size(), 0);
			_hash.Final (_result.data());
		}

		return _result;
	}


template<class H>
	inline BlobView
	CryptoPPHash<H>::view_result()
	{
		result();
		return _result;
	}


template<class H>
	inline void
	CryptoPPHash<H>::reset()
	{
		_hash.Restart();
		_result.clear();
	}

} // namespace neutrino

#endif


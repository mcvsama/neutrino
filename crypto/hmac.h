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
 * HMAC function
 */
class HMAC
{
  public:
	using Key = StrongType<Blob, struct KeyType>;

  public:
	// Ctor
	explicit
	HMAC (Key const& key, Blob const& message, Hash::Algorithm);

	/**
	 * Return the HMAC.
	 */
	Blob const&
	result() const;

	/**
	 * Return iterator to the beginning of the result.
	 */
	Blob::const_iterator
	begin() const;

	/**
	 * Return past-the-end iterator of the result.
	 */
	Blob::const_iterator
	end() const;

  private:
	Blob _result;
};


inline
HMAC::HMAC (Key const& pkey, Blob const& message, Hash::Algorithm algorithm)
{
	Hash h1 (algorithm);
	Hash h2 (algorithm);
	Blob key = *pkey;

	auto const block_size = h1.block_size();

	if (key.size() > block_size)
		key = hash (algorithm, key);

	if (key.size() < block_size)
		key.resize (block_size, 0);

	Blob opad (block_size, 0x5c);

	for (size_t i = 0; i < block_size; ++i)
		opad[i] ^= key[i];

	Blob ipad (block_size, 0x36);

	for (size_t i = 0; i < block_size; ++i)
		ipad[i] ^= key[i];

	ipad.insert (ipad.end(), message.begin(), message.end());
	h1.update (ipad);
	auto h1_result = h1.result();
	opad.insert (opad.end(), h1_result.begin(), h1_result.end());
	h2.update (opad);
	_result = h2.result();
}


inline Blob const&
HMAC::result() const
{
	return _result;
}


inline Blob::const_iterator
HMAC::begin() const
{
	return _result.cbegin();
}


inline Blob::const_iterator
HMAC::end() const
{
	return _result.cend();
}

} // namespace neutrino

#endif


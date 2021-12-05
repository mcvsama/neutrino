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

// Standard:
#include <cstddef>
#include <optional>
#include <span>

// Neutrino:
#include <neutrino/blob.h>
#include <neutrino/exception.h>
#include <neutrino/noncopyable.h>
#include <neutrino/nonmovable.h>


namespace neutrino {

/**
 * Generic hash-function interface.
 */
class Hash
{
  public:
	class AlreadyFinalized: public Exception
	{
	  public:
		AlreadyFinalized():
			Exception ("Hash object already finalized")
		{ }
	};

  public:
	enum Algorithm
	{
		SHA2_256,
		SHA3_224,
		SHA3_256,
		SHA3_384,
		SHA3_512,
	};

  public:
	// Ctor
	explicit
	Hash() = default;

	// Copy-ctor
	Hash (Hash const&) = default;

	// Move-ctor
	Hash (Hash&&) = default;

	// Dtor
	virtual
	~Hash() = default;

	// Copy-assignment operator:
	Hash&
	operator= (Hash const&) = default;

	// Move-assignment operator:
	Hash&
	operator= (Hash&&) = default;

	/**
	 * Alias to reset_and_calculate_result().
	 */
	Blob
	operator() (BlobView const blob)
		{ return reset_and_calculate_result (blob); }

	/**
	 * Update hash with new data.
	 */
	virtual void
	update (BlobView) = 0;

	/**
	 * Finalize and return hash result aka digest.
	 */
	virtual Blob
	result() = 0;

	/**
	 * Finalize and return view of the resulting hash.
	 */
	virtual BlobView
	view_result() = 0;

	/**
	 * Shortcut for resetting, calculating and returning result.
	 */
	Blob
	reset_and_calculate_result (BlobView);

	/**
	 * Return true if hash has been already finalized and read.
	 */
	virtual bool
	finalized() const = 0;

	/**
	 * Return block-size for the hash function used.
	 */
	virtual size_t
	block_size() const = 0;

	/**
	 * Return result-size for the hash function used.
	 */
	virtual size_t
	result_size() const = 0;

	/**
	 * Reset the hash function so it can be used again like newly created.
	 */
	virtual void
	reset() = 0;
};


inline Blob
Hash::reset_and_calculate_result (BlobView const blob)
{
	reset();
	update (blob);
	return result();
}

} // namespace neutrino


#include "hash_cryptopp.h"


namespace neutrino {

template<Hash::Algorithm Algorithm>
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
	inline Blob
	calculate_hash (BlobView const data)
	{
		auto hasher = get_hash_function<Algorithm>();
		hasher.update (data);
		return Blob (hasher.result());
	}

} // namespace neutrino

#endif


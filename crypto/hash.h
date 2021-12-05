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
class Hash:
	private Noncopyable,
	private Nonmovable
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

	// Dtor
	virtual
	~Hash() = default;

	/**
	 * Update hash with new data.
	 */
	virtual void
	update (BlobView) = 0;

	/**
	 * Finalize and return hash result aka digest.
	 */
	virtual BlobView
	result() = 0;

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
};

} // namespace neutrino


#include "hash_cryptopp.h"


namespace neutrino {

inline std::unique_ptr<Hash>
get_hash_function (Hash::Algorithm const algorithm)
{
	switch (algorithm)
	{
		case Hash::SHA2_256:
			return std::make_unique<HashSHA2_256>();

		case Hash::SHA3_224:
			return std::make_unique<HashSHA3_224>();

		case Hash::SHA3_256:
			return std::make_unique<HashSHA3_256>();

		case Hash::SHA3_384:
			return std::make_unique<HashSHA3_384>();

		case Hash::SHA3_512:
			return std::make_unique<HashSHA3_512>();

		default:
			std::terminate();
	}
}


inline Blob
calculate_hash (Hash::Algorithm algorithm, BlobView const data)
{
	auto const hasher = get_hash_function (algorithm);
	hasher->update (data);
	return Blob (hasher->result());
}

} // namespace neutrino

#endif


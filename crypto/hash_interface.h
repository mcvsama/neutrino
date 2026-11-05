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

#ifndef NEUTRINO__CRYPTO__HASH_INTERFACE_H__INCLUDED
#define NEUTRINO__CRYPTO__HASH_INTERFACE_H__INCLUDED

// Neutrino:
#include <neutrino/blob.h>
#include <neutrino/exception.h>
#include <neutrino/polymorphic.h>

// Standard:
#include <cstddef>


namespace neutrino {

/**
 * Generic hash-function interface.
 */
class Hash: public Polymorphic
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
	Hash (Hash&&) noexcept = default;

	// Copy-assignment operator:
	Hash&
	operator= (Hash const&) = default;

	// Move-assignment operator:
	Hash&
	operator= (Hash&&) = default;

	/**
	 * Alias to reset_and_compute_result().
	 */
	[[nodiscard]]
	Blob
	operator() (BlobView const blob)
		{ return reset_and_compute_result (blob); }

	/**
	 * Update hash with new data.
	 */
	virtual void
	update (BlobView) = 0;

	/**
	 * Finalize and return hash result aka digest.
	 */
	[[nodiscard]]
	virtual Blob
	result() = 0;

	/**
	 * Finalize and return view of the resulting hash.
	 */
	[[nodiscard]]
	virtual BlobView
	view_result() = 0;

	/**
	 * Shortcut for resetting, calculating and returning result.
	 */
	[[nodiscard]]
	Blob
	reset_and_compute_result (BlobView);

	/**
	 * Return true if hash has been already finalized and read.
	 */
	[[nodiscard]]
	virtual bool
	finalized() const = 0;

	/**
	 * Return block-size for the hash function used.
	 */
	[[nodiscard]]
	virtual size_t
	block_size() const = 0;

	/**
	 * Return result-size for the hash function used.
	 */
	[[nodiscard]]
	virtual size_t
	result_size() const = 0;

	/**
	 * Reset the hash function so it can be used again like newly created.
	 */
	virtual void
	reset() = 0;
};


inline Blob
Hash::reset_and_compute_result (BlobView const blob)
{
	reset();
	update (blob);
	return result();
}

} // namespace neutrino

#endif

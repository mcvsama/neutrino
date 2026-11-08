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

#ifndef NEUTRINO__USE_COUNT_H__INCLUDED
#define NEUTRINO__USE_COUNT_H__INCLUDED

// Neutrino:
#include <neutrino/demangle.h>
#include <neutrino/stdexcept.h>

// Standard:
#include <atomic>
#include <cstddef>
#include <typeinfo>


namespace neutrino {

/**
 * Counts dependencies on an externally owned object.
 * Counter operations and operations on distinct tokens sharing this counter may run concurrently.
 * The counter must outlive all its tokens and all operations on them.
 * Destruction of the counter and its owner must be coordinated externally;
 * observing get() == 0 does not prevent another thread from creating a token.
 * Counter operations do not synchronize access to the owner's data.
 */
class UseCount
{
  public:
	// Ctor without owner type information
	UseCount() = default;

	// Ctor with an explicit null owner
	explicit
	UseCount (std::nullptr_t):
		UseCount()
	{ }

	// Ctor using the owner's static type for diagnostics when available
	template<class T>
		explicit
		UseCount (T* owner_object);

	// Tokens refer to this counter by address, so it cannot be copied or moved.
	UseCount (UseCount const&) = delete;

	// Tokens refer to this counter by address, so it cannot be copied or moved.
	UseCount (UseCount&&) = delete;

	UseCount&
	operator= (UseCount const&) = delete;

	UseCount&
	operator= (UseCount&&) = delete;

	// Dtor
	~UseCount();

	/**
	 * Return a snapshot of the use count.
	 */
	std::size_t
	get() const noexcept
		{ return _counter.load (std::memory_order::relaxed); }

	/**
	 * Manually increase use count.
	 * Consider using UseToken instead.
	 */
	UseCount&
	operator++() noexcept;

	/**
	 * Manually decrease use count.
	 * Throw if count is already 0.
	 * Consider using UseToken instead.
	 */
	UseCount&
	operator--();

  private:
	std::atomic<std::size_t>	_counter		{ 0 };
	std::string					_object_type	{ "object" };
};


/**
 * RAII dependency on a UseCount.
 * Distinct tokens may be used concurrently, including copying a shared, unmodified source token.
 * Concurrent modification of the same token requires external synchronization.
 */
class UseToken
{
  public:
	// Ctor
	explicit
	UseToken (UseCount&);

	// Copy ctor
	UseToken (UseToken const&);

	// Dtor
	~UseToken()
		{ --*_use_count; }

	// Copy operator
	UseToken&
	operator= (UseToken const&);

  private:
	UseCount* _use_count;
};


template<class T>
	inline
	UseCount::UseCount (T* owner_object)
	{
		// typeid(T) requires complete class types; otherwise keep the generic diagnostic name.
		if constexpr (requires { typeid (T); })
			if (owner_object)
				_object_type = demangle (typeid (T).name());
	}


inline UseCount&
UseCount::operator++() noexcept
{
	_counter.fetch_add (1, std::memory_order::relaxed);
	return *this;
}


inline
UseToken::UseToken (UseCount& use_count):
	_use_count (&use_count)
{
	++*_use_count;
}


inline
UseToken::UseToken (UseToken const& other):
	_use_count (other._use_count)
{
	++*_use_count;
}

} // namespace neutrino

#endif

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

#ifndef NEUTRINO__CRYPTO__SECURE_H__INCLUDED
#define NEUTRINO__CRYPTO__SECURE_H__INCLUDED

// Neutrino:
#include <neutrino/crypto/utility.h>
#include <neutrino/blob.h>
#include <neutrino/concepts.h>

// Crypto++:
#include <cryptopp/misc.h>

// Standard:
#include <cstddef>
#include <string>
#include <type_traits>
#include <utility>


namespace neutrino {

/**
 * Securely wipes the active contents of the string or blob before destruction or replacement.
 * Moves copy the contents, then wipe and clear the source, including its small-string buffer.
 * Consequently, moving can allocate and throw just like copying.
 */
template<class Container>
	requires (std::is_same_v<Container, Blob> || std::is_same_v<Container, std::string>)
	class Secure
	{
	  public:
		// Ctor
		template<class ...Args>
			requires ((sizeof...(Args) != 1) || (NotCvrefSameAs<Args, Secure> && ...))
			Secure (Args&& ...args):
				_value (std::forward<Args> (args)...)
			{ }

		// Copy a consumed container before wiping it, including allocator and substring overloads.
		template<class ...Args>
			requires (std::is_constructible_v<Container, Container const&, Args...>)
			Secure (Container&& container, Args&& ...args):
				_value (std::as_const (container), std::forward<Args> (args)...)
			{
				wipe_and_clear (container);
			}

		// Copy ctor
		Secure (Secure const&) = default;

		// Move ctor
		Secure (Secure&& other):
			Secure (std::move (other._value))
		{ }

		// Dtor
		~Secure()
			{ wipe_and_clear (_value); }

		// Copy operator
		Secure&
		operator= (Secure const& other)
		{
			if (this != &other)
			{
				wipe_and_clear (_value);
				_value = other._value;
			}
			return *this;
		}

		// Move operator
		Secure&
		operator= (Secure&& other)
		{
			if (this != &other)
			{
				wipe_and_clear (_value);
				_value = other._value;
				wipe_and_clear (other._value);
			}
			return *this;
		}

		[[nodiscard]]
		Container const&
		operator*() const noexcept
			{ return _value; }

		[[nodiscard]]
		Container const*
		operator->() const noexcept
			{ return &_value; }

	  private:
		static void
		wipe_and_clear (Container& container) noexcept
		{
			CryptoPP::SecureWipeBuffer (container.data(), container.size());
			container.clear();
		}

	  private:
		Container _value;
	};

} // namespace neutrino

#endif

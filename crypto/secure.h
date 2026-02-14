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

// Standard:
#include <cstddef>
#include <type_traits>
#include <utility>


namespace neutrino {

/**
 * Wipes the string or blob on destruction.
 * Far from perfect, but still decreases chances of getting
 * key off RAM a bit.
 */
template<class Container>
	requires (std::is_same_v<Container, Blob> || std::is_same_v<Container, std::string>)
	class Secure
	{
	  public:
		// Ctor
		template<class ...Args>
			Secure (Args&& ...args):
				_value (std::forward<Args> (args)...)
			{ }

		// Ctor
		Secure (Container&& container):
			_value (std::move (container))
		{ }

		// Copy ctor
		Secure (Secure const&) = default;

		// Move ctor
		Secure (Secure&&) = default;

		// Dtor
		~Secure()
			{ wipe (_value); }

		// Copy operator
		Secure&
		operator= (Secure const& other)
		{
			wipe (_value);
			_value = other._value;
			return *this;
		}

		// Move operator
		Secure&
		operator= (Secure&& other)
		{
			wipe (_value);
			_value = std::move (other._value);
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
		Container _value;
	};

} // namespace neutrino

#endif

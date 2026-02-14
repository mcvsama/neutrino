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
#include <cstddef>
#include <typeinfo>


namespace neutrino {

class UseCount
{
  public:
	// Ctor
	template<class T>
		explicit
		UseCount (T* owner_object = nullptr);

	// Dtor
	~UseCount();

	/**
	 * Return use count.
	 */
	std::size_t
	get() const noexcept
		{ return _counter; }

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
	std::size_t		_counter		{ 0 };
	std::string		_object_type;
};


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
	UseCount::UseCount (T* owner_object):
		_object_type (owner_object ? demangle (typeid (owner_object).name()) : "object")
	{ }


inline UseCount&
UseCount::operator++() noexcept
{
	++_counter;
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

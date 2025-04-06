/* vim:ts=4
 *
 * Copyleft 2025  Michał Gawron
 * Marduk Unix Labs, http://mulabs.org/
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Visit http://www.gnu.org/licenses/gpl-3.0.html for more information on licensing.
 */

#ifndef NEUTRINO__VALUE_OR_PTR_H__INCLUDED
#define NEUTRINO__VALUE_OR_PTR_H__INCLUDED

// Standard:
#include <cstddef>
#include <optional>
#include <tuple>
#include <type_traits>
#include <utility>


namespace neutrino {

template<class pValue, class ...Args>
	class ValueOrPtr
	{
	  public:
		using Value		= pValue;
		using ArgsTuple	= std::tuple<Args...>;

	  public:
		/**
		 * Construct internal value.
		 */
		ValueOrPtr()
			requires std::is_default_constructible_v<Value>;

		/**
		 * Construct internal value.
		 */
		ValueOrPtr (std::in_place_t, Args&& ...args);

		/**
		 * Construct external pointer, or (if it's nullptr)
		 * construct an internal value.
		 */
		ValueOrPtr (Value* value)
			requires std::is_default_constructible_v<Value>;

		/**
		 * Construct external pointer, or (if it's nullptr)
		 * construct an internal value.
		 */
		ValueOrPtr (Value* value, std::in_place_t, Args&& ...args);

		// Copy ctor
		// Can be implemented in future, not needed now.
		ValueOrPtr (ValueOrPtr const&) = delete;

		// Move ctor
		// Can be implemented in future, not needed now.
		ValueOrPtr (ValueOrPtr&&) = delete;

		// Copy operator
		// Can be implemented in future, not needed now.
		ValueOrPtr&
		operator= (ValueOrPtr const&) = delete;

		// Move operator
		// Can be implemented in future, not needed now.
		ValueOrPtr&
		operator= (ValueOrPtr&&) = delete;

		ValueOrPtr&
		operator= (Value* value);

		[[nodiscard]]
		Value&
		operator*() noexcept
			{ return *_value_ptr; }

		[[nodiscard]]
		Value const&
		operator*() const noexcept
			{ return *_value_ptr; }

		[[nodiscard]]
		Value*
		operator->() noexcept
			{ return _value_ptr; }

		[[nodiscard]]
		Value const*
		operator->() const noexcept
			{ return _value_ptr; }

	  private:
		void
		construct_internal_from_saved_args();

	  private:
		ArgsTuple				_args;
		std::optional<Value>	_internal_value	{ std::nullopt };
		Value*					_value_ptr		{ nullptr };
	};


template<class V, class ...Args>
	inline
	ValueOrPtr<V, Args...>::ValueOrPtr()
		requires std::is_default_constructible_v<Value>
	{
		construct_internal_from_saved_args();
	}


template<class V, class ...Args>
	inline
	ValueOrPtr<V, Args...>::ValueOrPtr (std::in_place_t, Args&& ...args):
		_args (std::forward<Args> (args)...)
	{
		construct_internal_from_saved_args();
	}


template<class V, class ...Args>
	inline
	ValueOrPtr<V, Args...>::ValueOrPtr (Value* value)
		requires std::is_default_constructible_v<Value>:
		_value_ptr (value)
	{
		if (!_value_ptr)
			_value_ptr = &_internal_value.emplace();
	}


template<class V, class ...Args>
	inline
	ValueOrPtr<V, Args...>::ValueOrPtr (Value* value, std::in_place_t, Args&& ...args):
		_args (std::forward<Args> (args)...),
		_value_ptr (value)
	{
		if (!_value_ptr)
			construct_internal_from_saved_args();
	}


template<class V, class ...Args>
	inline ValueOrPtr<V, Args...>&
	ValueOrPtr<V, Args...>::operator= (Value* value)
	{
		if (value)
		{
			_internal_value.reset();
			_value_ptr = value;
		}
		else
			construct_internal_from_saved_args();

		return *this;
	}


template<class V, class ...Args>
	inline void
	ValueOrPtr<V, Args...>::construct_internal_from_saved_args()
	{
		std::apply ([this] (auto const& ...tupleArgs) {
			_value_ptr = &_internal_value.emplace (tupleArgs...);
		}, _args);
	}

} // namespace neutrino

#endif


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

#ifndef NEUTRINO__SYNCHRONIZED_H__INCLUDED
#define NEUTRINO__SYNCHRONIZED_H__INCLUDED

// Standard:
#include <cstddef>
#include <mutex>
#include <type_traits>


namespace neutrino {

template<class V, class M>
	class Synchronized;


/**
 * This object allows you to access the resource protected by Synchronized.
 * As long as it exists, the lock is held.
 *
 * You don't create it yourself, instead you use
 * Synchronized<T>::lock() method.
 */
template<class pValue, class pMutex>
	class UniqueAccessor
	{
		template<class V, class M>
			friend class Synchronized;

	  public:
		// std::remove_reference_t<> allows using reference type as Value:
		using Value			= pValue;
		using NonRefValue	= std::remove_reference_t<Value>;
		using Mutex			= pMutex;

	  private:
		/**
		 * Constructor for UniqueAccessor for non-const values.
		 */
		explicit
		UniqueAccessor (Synchronized<Value, Mutex>& synchronized)
			requires (!std::is_const_v<Value>):
			_value (&synchronized._value),
			_lock (synchronized._mutex)
		{ }

		/**
		 * Constructor for UniqueAccessor for const values.
		 */
		explicit
		UniqueAccessor (Synchronized<std::remove_const_t<Value>, Mutex> const& synchronized)
			requires (std::is_const_v<Value>):
			_value (&synchronized._value),
			_lock (synchronized._mutex)
		{ }

	  public:
		// Move ctor
		UniqueAccessor (UniqueAccessor&&) = default;

		// Move operator
		UniqueAccessor&
		operator= (UniqueAccessor&&) noexcept = default;

		/**
		 * Access the value by reference.
		 */
		NonRefValue&
		operator*() noexcept
			requires (!std::is_const_v<Value>)
		{ return *_value; }

		/**
		 * Access the value by reference.
		 */
		NonRefValue const&
		operator*() const noexcept
			{ return *_value; }

		/**
		 * Access the value by pointer.
		 */
		NonRefValue*
		operator->() noexcept
			requires (!std::is_const_v<NonRefValue>)
		{ return _value; }

		/**
		 * Access the value by pointer.
		 */
		NonRefValue const*
		operator->() const noexcept
			{ return _value; }

		/**
		 * Unlock the mutex and deassociate this Accessor from a Synchronized object.
		 * After calling this function, calling dereference operators is undefined-behaviour.
		 */
		void
		unlock() noexcept;

	  private:
		NonRefValue*			_value;
		std::unique_lock<Mutex>	_lock;
	};


/**
 * RAII-style safe lock. You need a token to access the resource, and if token exists, it guarantees
 * that the resource is locked.
 *
 * \param	pValue
 *			Type of balue to be protected by lock.
 * \param	pMutex
 *			One of standard locks (eg. std::mutex, etc) that can be dealt with with std::unique_lock<pMutex>.
 */
template<class pValue, class pMutex = std::mutex>
	class Synchronized
	{
		template<class V, class M>
			friend class UniqueAccessor;

	  public:
		using Value			= pValue;
		using NonRefValue	= std::remove_reference_t<Value>;
		using Mutex			= pMutex;

	  public:
		// Ctor
		template<class ...Args>
			Synchronized (Args&& ...args) noexcept (noexcept (Value (std::forward<Args> (args)...))):
				_value (std::forward<Args> (args)...)
			{ }

		// Copy ctor
		Synchronized (Synchronized const&);

		// Move ctor
		Synchronized (Synchronized&&) = delete;

		// Copy operator
		Synchronized&
		operator= (Synchronized const&);

		// Move ctor
		Synchronized&
		operator= (Synchronized&&) = delete;

		/**
		 * Atomically replaces the current value with `desired`.
		 */
		void
		store (NonRefValue const& desired)
			{ *lock() = desired; }

		/**
		 * Atomically loads and returns the current value of the protected variable.
		 */
		[[nodiscard]]
		NonRefValue
		load() const
			{ return *lock(); }

		/**
		 * Atomically replaces the underlying value with desired (a read-modify-write operation).
		 */
		[[nodiscard]]
		NonRefValue
		exchange (NonRefValue const& desired);

		/**
		 * Return unique access token.
		 */
		[[nodiscard("you must hold the returned object as long as you need the lock to be locked")]]
		UniqueAccessor<Value, Mutex>
		lock()
			{ return UniqueAccessor<Value, Mutex> (*this); }

		/**
		 * Return const unique access token.
		 */
		[[nodiscard("you must hold the returned object as long as you need the lock to be locked")]]
		UniqueAccessor<Value const, Mutex>
		lock() const
			{ return UniqueAccessor<Value const, Mutex> (*this); }

		/**
		 * Shorthand for lock()->...
		 */
		[[nodiscard]]
		UniqueAccessor<Value, Mutex>
		operator->() noexcept
			{ return lock(); }

	  private:
		Value			_value;
		Mutex mutable	_mutex;
	};


template<class V, class M>
	inline void
	UniqueAccessor<V, M>::unlock() noexcept
	{
		_value = nullptr;
		_lock.unlock();
	}


template<class V, class M>
	inline
	Synchronized<V, M>::Synchronized (Synchronized const& other):
		_value (*other.lock())
	{ }


template<class V, class M>
	inline Synchronized<V, M>&
	Synchronized<V, M>::operator= (Synchronized const& other)
	{
		*lock() = *other.lock();
		return *this;
	}


template<class V, class M>
	inline Synchronized<V, M>::NonRefValue
	Synchronized<V, M>::exchange (NonRefValue const& desired)
	{
		auto accessor = lock();
		auto old = *accessor;
		*accessor = desired;
		return old;
	}

} // namespace neutrino

#endif

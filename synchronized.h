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
#include <concepts>
#include <cstddef>
#include <memory>
#include <mutex>
#include <type_traits>
#include <utility>


namespace neutrino {

template<class V, class M>
	class Synchronized;


/**
 * This object allows you to access the resource protected by Synchronized.
 * It holds the lock until unlocked or destroyed, except while waiting on a condition variable.
 * The Synchronized object must outlive this accessor.
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
		 * Lock the mutex and associate this accessor with the protected object.
		 */
		explicit
		UniqueAccessor (NonRefValue* value, Mutex& mutex):
			_value (value),
			_lock (mutex)
		{ }

	  public:
		// Move ctor
		UniqueAccessor (UniqueAccessor&&) noexcept (std::is_nothrow_move_constructible_v<std::unique_lock<Mutex>>) = default;

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
		 * Wait until the predicate is true, temporarily releasing the lock while waiting.
		 * The predicate is evaluated with the lock held, and the lock is held again on return.
		 * The condition variable must accept std::unique_lock<Mutex>.
		 * This accessor must own the lock.
		 */
		template<class Condition, class Predicate>
			void
			wait (Condition& condition, Predicate&& predicate)
				{ condition.wait (_lock, std::forward<Predicate> (predicate)); }

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
 * RAII-style safe lock. You need a token to access the resource. The token holds the lock
 * until unlocked or destroyed, except while waiting on a condition variable.
 *
 * \param	pValue
 *			Type of value to be protected by lock.
 *			For reference types, the referenced object must outlive this wrapper and its accessors.
 *			Copy construction and copy assignment are disabled for reference types.
 * \param	pMutex
 *			Mutex type usable with std::unique_lock<pMutex>.
 *			Copy assignment also requires try_lock() for deadlock-free locking.
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
		// Ctor for owned values
		// Exclude a single argument of this Synchronized specialization so forwarding
		// doesn't take precedence over the copy constructor for non-const lvalues.
		// Other arguments must be usable to construct Value.
		template<class ...Args>
			requires (
				!std::is_reference_v<Value> &&
				!(sizeof...(Args) == 1 &&
					(std::is_same_v<std::remove_cvref_t<Args>, Synchronized> || ...)) &&
				std::is_constructible_v<Value, Args&&...> &&
				std::is_default_constructible_v<Mutex>
			)
			Synchronized (Args&& ...args)
				noexcept (
					std::is_nothrow_constructible_v<Value, Args&&...> &&
					std::is_nothrow_default_constructible_v<Mutex>
				):
				_value (std::forward<Args> (args)...)
			{ }

		// Reference ctor
		// Only borrow from lvalues, and reject conversions that create temporary objects.
		template<class Arg>
			requires (
				std::is_lvalue_reference_v<Value> &&
				std::is_lvalue_reference_v<Arg&&> &&
				!std::is_same_v<std::remove_cvref_t<Arg>, Synchronized> &&
				std::is_constructible_v<Value, Arg&&> &&
				!std::reference_constructs_from_temporary_v<Value, Arg&&> &&
				std::is_default_constructible_v<Mutex>
			)
			Synchronized (Arg&& arg)
				noexcept (
					std::is_nothrow_constructible_v<Value, Arg&&> &&
					std::is_nothrow_default_constructible_v<Mutex>
				):
				_value (std::forward<Arg> (arg))
			{ }

		// Copy ctor
		Synchronized (Synchronized const&)
			requires (
				!std::is_reference_v<Value> &&
				std::is_constructible_v<Value, NonRefValue const&> &&
				std::is_default_constructible_v<Mutex>
			);

		// Move ctor
		Synchronized (Synchronized&&) = delete;

		// Copy operator
		Synchronized&
		operator= (Synchronized const&)
			requires (
				!std::is_reference_v<Value> &&
				std::is_assignable_v<NonRefValue&, NonRefValue const&> &&
				requires (Mutex& mutex) {
					{ mutex.try_lock() } -> std::convertible_to<bool>;
				}
			);

		// Move ctor
		Synchronized&
		operator= (Synchronized&&) = delete;

		/**
		 * Atomically replaces the current value with `desired`.
		 */
		void
		store (NonRefValue const& desired)
			requires (std::is_assignable_v<NonRefValue&, NonRefValue const&>)
		{ *lock() = desired; }

		/**
		 * Atomically loads and returns the current value of the protected variable.
		 * The result must be implicitly constructible from a const reference to the value.
		 */
		[[nodiscard]]
		NonRefValue
		load() const
			requires (std::is_convertible_v<NonRefValue const&, NonRefValue>)
		{ return *lock(); }

		/**
		 * Atomically replaces the underlying value with desired (a read-modify-write operation).
		 */
		[[nodiscard]]
		NonRefValue
		exchange (NonRefValue const& desired)
			requires (
				std::is_assignable_v<NonRefValue&, NonRefValue const&> &&
				std::is_convertible_v<NonRefValue&, std::remove_cv_t<NonRefValue>> &&
				// Match the move-or-copy return path used in the implementation.
				(std::is_move_constructible_v<std::remove_cv_t<NonRefValue>>
					? std::is_convertible_v<std::remove_cv_t<NonRefValue>&&, NonRefValue>
					: std::is_convertible_v<std::remove_cv_t<NonRefValue> const&, NonRefValue>)
			);

		/**
		 * Return unique access token.
		 */
		[[nodiscard("you must hold the returned object as long as you need the lock to be locked")]]
		UniqueAccessor<NonRefValue, Mutex>
		lock() &
			{ return UniqueAccessor<NonRefValue, Mutex> (std::addressof (_value), _mutex); }

		/**
		 * Return a read-only access token, including when Value is a reference.
		 */
		[[nodiscard("you must hold the returned object as long as you need the lock to be locked")]]
		UniqueAccessor<NonRefValue const, Mutex>
		lock() const&
			{ return UniqueAccessor<NonRefValue const, Mutex> (std::addressof (_value), _mutex); }

		// A temporary wrapper would be destroyed while the accessor still holds its mutex.
		UniqueAccessor<NonRefValue, Mutex>
		lock() && = delete;

		UniqueAccessor<NonRefValue const, Mutex>
		lock() const&& = delete;

		/**
		 * Shorthand for lock()->...
		 */
		[[nodiscard]]
		UniqueAccessor<NonRefValue, Mutex>
		operator->() &
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
	Synchronized<V, M>::Synchronized (Synchronized const& other)
		requires (
			!std::is_reference_v<Value> &&
			std::is_constructible_v<Value, NonRefValue const&> &&
			std::is_default_constructible_v<Mutex>
		):
		_value (*other.lock())
	{ }


template<class V, class M>
	inline Synchronized<V, M>&
	Synchronized<V, M>::operator= (Synchronized const& other)
		requires (
			!std::is_reference_v<Value> &&
			std::is_assignable_v<NonRefValue&, NonRefValue const&> &&
			requires (Mutex& mutex) {
				{ mutex.try_lock() } -> std::convertible_to<bool>;
			}
		)
	{
		if (this == std::addressof (other))
			return *this;

		std::scoped_lock guard (_mutex, other._mutex);
		_value = std::as_const (other._value);
		return *this;
	}


template<class V, class M>
	inline Synchronized<V, M>::NonRefValue
	Synchronized<V, M>::exchange (NonRefValue const& desired)
		requires (
			std::is_assignable_v<NonRefValue&, NonRefValue const&> &&
			std::is_convertible_v<NonRefValue&, std::remove_cv_t<NonRefValue>> &&
			(std::is_move_constructible_v<std::remove_cv_t<NonRefValue>>
				? std::is_convertible_v<std::remove_cv_t<NonRefValue>&&, NonRefValue>
				: std::is_convertible_v<std::remove_cv_t<NonRefValue> const&, NonRefValue>)
		)
	{
		auto accessor = lock();
		auto old = *accessor;
		*accessor = desired;

		// An explicitly deleted move constructor would be selected by `return old`.
		// Copy in that case, preserving NRVO for types that support the usual return.
		if constexpr (std::is_move_constructible_v<decltype (old)>)
			return old;
		else
			return std::as_const (old);
	}

} // namespace neutrino

#endif

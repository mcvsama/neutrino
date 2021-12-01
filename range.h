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

#ifndef NEUTRINO__RANGE_H__INCLUDED
#define NEUTRINO__RANGE_H__INCLUDED

// Standard:
#include <concepts>
#include <cstddef>


namespace neutrino {

template<class T>
	concept RangeValueConcept =
		std::swappable<T> &&
		std::copyable<T> &&
		std::copy_constructible<T> &&
		std::default_initializable<T> &&
		requires (T a, T b) {
			a + b;
			a - b;
			0.5 * (a + b);
			std::min (a, b);
		};


template<RangeValueConcept tValueType>
	class Range
	{
	  public:
		typedef tValueType ValueType;

	  public:
		constexpr
		Range() noexcept = default;

		constexpr
		Range (ValueType min, ValueType max) noexcept;

		constexpr
		Range (Range const&) noexcept;

		template<class OtherType>
			constexpr
			operator Range<OtherType>() const noexcept
			{
				return Range<OtherType> (_min, _max);
			}

		constexpr Range&
		operator= (Range const&) = default;

		constexpr ValueType
		min() const noexcept;

		constexpr ValueType
		max() const noexcept;

		void
		set_min (ValueType min) noexcept;

		void
		set_max (ValueType max) noexcept;

		/**
		 * Return maximum() - minimum().
		 */
		constexpr ValueType
		extent() const noexcept;

		/**
		 * Return 0.5 * (min() + max()).
		 */
		constexpr ValueType
		mid() const noexcept;

		/**
		 * Swap minimum and maximum values.
		 */
		void
		flip() noexcept;

		/**
		 * Return a copy with swapped minimum and maximum values.
		 */
		constexpr Range
		flipped() const noexcept;

		/**
		 * Return true if given value fits inside range,
		 * inclusively.
		 */
		template<class = void>
			constexpr bool
			includes (ValueType) const noexcept
				requires requires (ValueType a, ValueType b) { a <= b; };

		/**
		 * Creates a new range with minimum value lesser of the two (this and other) and
		 * maximum value which is greater of the two.
		 */
		constexpr Range
		extended (Range other) const;

	  private:
		ValueType	_min {};
		ValueType	_max {};
	};


template<RangeValueConcept T>
	constexpr
	Range<T>::Range (ValueType min, ValueType max) noexcept:
		_min (min),
		_max (max)
	{ }


template<RangeValueConcept T>
	constexpr
	Range<T>::Range (Range<T> const& other) noexcept:
		_min (other._min),
		_max (other._max)
	{ }


template<RangeValueConcept T>
	constexpr
	typename Range<T>::ValueType
	Range<T>::min() const noexcept
	{
		return _min;
	}


template<RangeValueConcept T>
	constexpr
	typename Range<T>::ValueType
	Range<T>::max() const noexcept
	{
		return _max;
	}


template<RangeValueConcept T>
	void
	Range<T>::set_min (ValueType min) noexcept
	{
		_min = min;
	}


template<RangeValueConcept T>
	void
	Range<T>::set_max (ValueType max) noexcept
	{
		_max = max;
	}


template<RangeValueConcept T>
	constexpr
	typename Range<T>::ValueType
	Range<T>::extent() const noexcept
	{
		return _max - _min;
	}


template<RangeValueConcept T>
	constexpr
	typename Range<T>::ValueType
	Range<T>::mid() const noexcept
	{
		return 0.5 * (_min + _max);
	}


template<RangeValueConcept T>
	void
	Range<T>::flip() noexcept
	{
		std::swap (_min, _max);
	}


template<RangeValueConcept T>
	constexpr Range<T>
	Range<T>::flipped() const noexcept
	{
		return Range { _max, _min };
	}


template<RangeValueConcept T>
	template<class>
		constexpr bool
		Range<T>::includes (ValueType value) const noexcept
			requires requires (ValueType a, ValueType b) { a <= b; }
		{
			return (_min <= value && value <= _max) || (_max <= value && value <= _min);
		}


template<RangeValueConcept T>
	constexpr Range<T>
	Range<T>::extended (Range<T> other) const
	{
		return { std::min (min(), other.min()), std::max (max(), other.max()) };
	}

} // namespace neutrino

#endif


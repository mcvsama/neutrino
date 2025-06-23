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
			{ a + b } -> std::same_as<T>;
			{ a - b } -> std::same_as<T>;
			0.5 * (a + b);
		};


template<class T>
	concept ComparableRangeValueConcept =
		RangeValueConcept<T> &&
		requires (T a, T b) {
			a <= b;
			std::min (a, b);
		};


/**
 * Base class not having min/max fields for values that aren't comparable.
 */
template<class tValue, class = void>
	class RangeBase
	{
	  public:
		constexpr
		RangeBase() = default;
	};


/**
 * Base class with min/max fields for values that are comparable.
 */
template<class tValue>
	class RangeBase<tValue, std::enable_if_t<ComparableRangeValueConcept<tValue>>>
	{
	  public:
		typedef tValue Value;

	  public:
		constexpr
		RangeBase() = default;

		constexpr Value const&
		min() const noexcept
			{ return _min; }

		constexpr Value const&
		max() const noexcept
			{ return _max; }

		/**
		 * Return max() - min(). Always non-negative.
		 */
		constexpr Value
		extent() const noexcept
			{ return _max - _min; }

		/**
		 * Return true if given value fits inside range,
		 * inclusively.
		 */
		template<class = void>
			constexpr bool
			includes (Value const& value) const noexcept
				{ return _min <= value && value <= _max; }

	  protected:
		Value	_min;
		Value	_max;
	};


template<RangeValueConcept tValue>
	class Range: public RangeBase<tValue>
	{
	  public:
		typedef tValue Value;

	  public:
		constexpr
		Range() noexcept = default;

		constexpr
		Range (Value const& min, Value const& max) noexcept;

		constexpr
		Range (Range const&) noexcept;

		template<class OtherType>
			constexpr
			operator Range<OtherType>() const noexcept
			{
				return Range<OtherType> (_begin, _end);
			}

		constexpr Range&
		operator= (Range const&) = default;

		constexpr Value const&
		begin() const noexcept
			{ return _begin; }

		constexpr Value const&
		end() const noexcept
			{ return _end; }

		void
		set_begin (Value const& begin) noexcept;

		void
		set_end (Value const& end) noexcept;

		/**
		 * Return end() - begin().
		 */
		constexpr Value
		delta() const noexcept
			{ return _end - _begin; }

		/**
		 * Return 0.5 * (min() + max()).
		 */
		constexpr Value
		midpoint() const noexcept
			{ return 0.5 * (_begin + _end); }

		/**
		 * Swap minimum and maximum values.
		 */
		constexpr void
		flip() noexcept;

		/**
		 * Return a copy with swapped minimum and maximum values.
		 */
		constexpr Range
		flipped() const noexcept
			{ return Range { _end, _begin }; }

		/**
		 * Structured bindings support.
		 */
		template<std::size_t I>
			constexpr Value
			get() const noexcept
			{
				if constexpr (I == 0)
					return _begin;
				else if constexpr (I == 1)
					return _end;
				else
					throw std::logic_error ("Valid indices are 0 and 1");
			}

		/**
		 * Creates a new range with minimum value lesser of the two (this and other) and
		 * maximum value which is greater of the two.
		 */
		// TODO ill-defined if _begin > _end
		constexpr Range
		extended (Range other) const
			requires ComparableRangeValueConcept<Value>
		{ return { std::min (this->min(), other.min()), std::max (this->max(), other.max()) }; }

	  private:
		constexpr void
		update_min_max();

	  private:
		Value	_begin	{};
		Value	_end	{};
	};


template<RangeValueConcept T>
	constexpr
	Range<T>::Range (Value const& begin, Value const& end) noexcept:
		_begin (begin),
		_end (end)
	{
		update_min_max();
	}


template<RangeValueConcept T>
	constexpr
	Range<T>::Range (Range<T> const& other) noexcept:
		_begin (other._begin),
		_end (other._end)
	{
		update_min_max();
	}


template<RangeValueConcept T>
	void
	Range<T>::set_begin (Value const& begin) noexcept
	{
		_begin = begin;
		update_min_max();
	}


template<RangeValueConcept T>
	void
	Range<T>::set_end (Value const& end) noexcept
	{
		_end = end;
		update_min_max();
	}


template<RangeValueConcept T>
	constexpr void
	Range<T>::flip() noexcept
	{
		std::swap (_begin, _end);

		if constexpr (ComparableRangeValueConcept<Value>)
			std::swap (this->_min, this->_max);
	}


template<RangeValueConcept T>
	constexpr void
	Range<T>::update_min_max()
	{
		// No-op if the values aren't comparable:
		if constexpr (ComparableRangeValueConcept<Value>)
		{
			if (_begin <= _end)
			{
				this->_min = _begin;
				this->_max = _end;
			}
			else
			{
				this->_min = _end;
				this->_max = _begin;
			}
		}
	}

} // namespace neutrino


/*
 * Structured bindings support
 */


template<neutrino::RangeValueConcept T>
	struct std::tuple_size<neutrino::Range<T>>: std::integral_constant<std::size_t, 2>
	{ };


template<neutrino::RangeValueConcept T, std::size_t I>
	struct std::tuple_element<I, neutrino::Range<T>>
	{
		using type = T;
	};

#endif


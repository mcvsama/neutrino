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

#ifndef NEUTRINO__SI__UTILS_H__INCLUDED
#define NEUTRINO__SI__UTILS_H__INCLUDED

// Standard:
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>
#include <cmath>
#include <string_view>

// Boost:
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/case_conv.hpp>

/// Neutrino:
#include <neutrino/core_types.h>
#include <neutrino/endian.h>

// Local:
#include "concepts.h"
#include "quantity.h"
#include "standard_units.h"
#include "standard_quantities.h"


namespace neutrino::si {

/**
 * Primary template just gives the template argument.
 */
template<class T>
	struct decay_quantity
	{
		using type = T;
	};


/**
 * Specialization for Quantity-type gives Q::Value.
 */
template<QuantityConcept Q>
	struct decay_quantity<Q>
	{
		using type = typename Q::Value;
	};


template<class Q>
	using decay_quantity_t = typename decay_quantity<Q>::type;


/**
 * Return unit vector for given unit string.
 * Unit string should have form "kg^3 m-5".
 *
 * \throw	UnsupportedUnit
 *			If unit can't be parsed to any known unit.
 */
DynamicUnit
parse_unit (std::string_view);


/**
 * Returns unchanged argument for non-Quantity types and Quantity::base_value()
 * for Quantity types.
 */
template<class Any>
	constexpr decay_quantity_t<Any>
	base_value (Any const value) noexcept
	{
		if constexpr (QuantityConcept<Any>)
			return value.base_value();
		else
			return value;
	}


/**
 * Return quantity in units U if Q is a Quantity type.
 */
template<QuantityConcept Q, UnitConcept U = Q::Unit>
	constexpr auto
	quantity (Q const value) noexcept
	{
		return value.template in<U>();
	}


/**
 * Just return value.
 */
template<class T>
	requires (!QuantityConcept<T>)
	constexpr auto
	quantity (T const value) noexcept
	{
		return value;
	}


/**
 * Return quantity in given units if value is of Quantity type.
 * Otherwise return the argument (the string argument is ignored).
 *
 * \throw	UnsupportedUnit
 *			If unit_str can't be parsed correctly.
 * \throw	IncompatibleTypes
 *			If quantity can't be expressed in given units.
 */
template<class Any>
	constexpr decay_quantity_t<Any>
	quantity (Any const value, std::string_view const unit_str)
	{
		if constexpr (QuantityConcept<Any>)
			return quantity (value, parse_unit (unit_str));
		else
			return value;
	}


/**
 * Return quantity in given units if value is of Quantity type.
 * Return value argument if value has non-Quantity type (the DynamicUnit argument is ignored).
 *
 * \throw	IncompatibleTypes
 *			If quantity can't be expressed in given units.
 */
template<class Any>
	constexpr decay_quantity_t<Any>
	quantity (Any const value, DynamicUnit const& unit)
	{
		if constexpr (QuantityConcept<Any>)
			return convert (Any::Unit::dynamic_unit(), value.value(), unit);
		else
			return value;
	}


/**
 * Return quantity in units U, if value is of Quantity type.
 * Otherwise just return the argument.
 */
template<UnitConcept U, class Any>
	constexpr decay_quantity_t<Any>
	quantity_in_units (Any const value) noexcept
	{
		if constexpr (QuantityConcept<Any>)
			return quantity<Any, U> (value);
		else
			return value;
	}


/**
 * Convert to binary blob representing base unit.
 */
inline Blob
to_blob (QuantityConcept auto quantity)
{
	Blob result (sizeof (quantity.base_value()), 0);
	auto value = quantity.base_value();
	boost::endian::native_to_little_inplace (value);
	std::memcpy (result.data(), &value, sizeof (value));
	return result;
}


/**
 * std::to_string implementation for Quantity-types.
 */
template<UnitConcept pUnit, ValueConcept pValue>
	inline std::string
	to_string (Quantity<pUnit, pValue> const quantity)
	{
		return std::to_string (quantity.value()) + " " + UnitTraits<pUnit>::symbol();
	}


/**
 * Returns unit name of given quantity.
 */
template<UnitConcept pUnit, ValueConcept pValue>
	inline std::string
	unit_to_string (Quantity<pUnit, pValue>)
	{
		return UnitTraits<pUnit>::symbol();
	}


/**
 * std::to_string implementation.
 */
inline std::string
to_string (DynamicRatio const& dr)
{
	return std::string ("DynamicRatio<") + std::to_string (dr.numerator()) + " / " + std::to_string (dr.denominator()) + ">";
}


/**
 * std::to_string implementation.
 */
inline std::string
to_string (DynamicUnit const& du)
{
	std::string s = "DynamicUnit<";
	for (int exp: du.exponents())
		s += std::to_string (exp) + ", ";
	s += to_string (du.scale()) + ", " + to_string (du.offset());
	return s + ">";
}


/**
 * Convert from binary blob representing base unit.
 *
 * \throw	UnparsableValue
 *			When data can't be correctly parsed.
 */
template<UnitConcept pUnit, ValueConcept pValue>
	inline void
	parse (BlobView const blob, Quantity<pUnit, pValue>& quantity)
	{
		if (blob.size() == sizeof (pValue))
		{
			pValue parsed;
			std::memcpy (&parsed, blob.data(), sizeof (parsed));
			boost::endian::little_to_native_inplace (parsed);
			quantity = Quantity<NormalizedUnit<pUnit>, pValue> { parsed };
		}
		else
			throw UnparsableValue ("wrong size of binary data");
	}


/**
 * Returning version of parse (blob).
 */
template<QuantityConcept Q>
	inline Q
	parse (BlobView const blob)
	{
		Q result;
		parse (blob, result);
		return result;
	}


/**
 * Parse string to get quantity.
 * String should look like "1.23 kg m^3 s^-1", that is it should consist of a number
 * and a set of unit names and exponents. Short symbols (returned by UnitTraits<T>::symbol() are allowed.
 *
 * \throw	UnparsableValue
 *			If value can't be parsed correctly.
 * \throw	IncompatibleTypes
 *			If type is not convertible to given quantity argument.
 */
template<UnitConcept pUnit, ValueConcept pValue>
	inline void
	parse (std::string_view const& str, Quantity<pUnit, pValue>& quantity)
	{
		std::size_t p = str.find_first_of (' ');

		if (p == std::string_view::npos)
			throw UnparsableValue ("error while parsing: " + std::string (str));

		try {
			// float in "units" -> Quantity
			pValue value = boost::lexical_cast<pValue> (str.substr (0, p));
			std::string_view unit_str = str.substr (p + 1);

			DynamicUnit source_unit = parse_unit (unit_str);
			DynamicUnit target_unit = pUnit::dynamic_unit();

			quantity = Quantity<pUnit, pValue> (convert (source_unit, value, target_unit));
		}
		catch (IncompatibleTypes&)
		{
			throw;
		}
		catch (...)
		{
			throw UnparsableValue ("error while parsing: " + std::string (str));
		}
	}


/**
 * Returning version of parse (string).
 */
template<QuantityConcept Q>
	inline Q
	parse (std::string_view const& str)
	{
		Q result;
		parse (str, result);
		return result;
	}


/**
 * std::abs() equivalent
 */
template<int E0, int E1, int E2, int E3, int E4, int E5, int E6, int E7, ScaleConcept S, ValueConcept Value>
	constexpr auto
	abs (Quantity<Unit<E0, E1, E2, E3, E4, E5, E6, E7, S, std::ratio<0>>, Value> q) noexcept
	{
		return Quantity<Unit<E0, E1, E2, E3, E4, E5, E6, E7, S, std::ratio<0>>, Value> (std::abs (q.value()));
	}


/**
 * std::isinf() equivalent.
 */
template<int E0, int E1, int E2, int E3, int E4, int E5, int E6, int E7, ScaleConcept S, ValueConcept Value>
	constexpr auto
	isinf (Quantity<Unit<E0, E1, E2, E3, E4, E5, E6, E7, S, std::ratio<0>>, Value> q) noexcept
	{
		return std::isinf (q.value());
	}


/**
 * std::signbit() equivalent.
 */
template<int E0, int E1, int E2, int E3, int E4, int E5, int E6, int E7, ScaleConcept S, ValueConcept Value>
	constexpr auto
	signbit (Quantity<Unit<E0, E1, E2, E3, E4, E5, E6, E7, S, std::ratio<0>>, Value> q) noexcept
	{
		return std::signbit (q.value());
	}


/**
 * std::isnan() equivalent.
 */
template<int E0, int E1, int E2, int E3, int E4, int E5, int E6, int E7, ScaleConcept S, ValueConcept Value>
	constexpr auto
	isnan (Quantity<Unit<E0, E1, E2, E3, E4, E5, E6, E7, S, std::ratio<0>>, Value> q) noexcept
	{
		return std::isnan (q.value());
	}


/**
 * std::sqrt() equivalent
 */
template<int E0, int E1, int E2, int E3, int E4, int E5, int E6, int E7, ScaleConcept S, ValueConcept Value>
	requires (E0 % 2 == 0 && E1 % 2 == 0 && E2 % 2 == 0 && E3 % 2 == 0 && E4 % 2 == 0 && E5 % 2 == 0 && E6 % 2 == 0 && E7 % 2 == 0)
	constexpr auto
	sqrt (Quantity<Unit<E0, E1, E2, E3, E4, E5, E6, E7, S, std::ratio<0>>, Value> q) noexcept
	{
		typedef Quantity<Unit<E0, E1, E2, E3, E4, E5, E6, E7>, Value> NormalizedQuantity;

		auto value = std::sqrt (NormalizedQuantity (q).base_value());
		return Quantity<Unit<E0 / 2, E1 / 2, E2 / 2, E3 / 2, E4 / 2, E5 / 2, E6 / 2, E7 / 2, S, std::ratio<0>>, Value> (value);
	}


/**
 * std::isfinite() equivalent
 */
template<int E0, int E1, int E2, int E3, int E4, int E5, int E6, int E7, ScaleConcept S, ValueConcept Value>
	constexpr bool
	isfinite (Quantity<Unit<E0, E1, E2, E3, E4, E5, E6, E7, S, std::ratio<0>>, Value> q) noexcept
	{
		return std::isfinite (q.value());
	}


constexpr quantities::Angle::Value
sin (quantities::Angle a)
{
	return std::sin (a.in<units::Radian>());
}


constexpr quantities::Angle::Value
cos (quantities::Angle a)
{
	return std::cos (a.in<units::Radian>());
}


constexpr quantities::Angle::Value
tan (quantities::Angle a)
{
	return std::tan (a.in<units::Radian>());
}


constexpr quantities::Angle::Value
atan2 (auto const y, auto const x)
{
	return std::atan2 (quantity (y), quantity (x));
}


/**
 * std::ostream support.
 */
template<UnitConcept pUnit, ValueConcept pValue>
	inline std::ostream&
	operator<< (std::ostream& out, Quantity<pUnit, pValue> quantity)
	{
		std::ptrdiff_t const additional_size = 1 + static_cast<std::ptrdiff_t> (UnitTraits<pUnit>::symbol().size());
		std::ptrdiff_t const w = out.width();

		return out << std::setw (std::max<std::ptrdiff_t> (0, w - additional_size)) << quantity.value() << " " << UnitTraits<pUnit>::symbol();
	}

} // namespace neutrino::si

#endif


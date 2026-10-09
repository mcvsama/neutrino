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

#ifndef NEUTRINO__SI__CONVERSION_H__INCLUDED
#define NEUTRINO__SI__CONVERSION_H__INCLUDED

// Local:
#include "concepts.h"
#include "predicates.h"
#include "standard_units.h"
#include "unit.h"

// Standard:
#include <cstddef>


namespace neutrino::si {

// Forward
template<UnitConcept pUnit, ValueConcept pValue>
	class Quantity;


namespace detail {

/**
 * Convert a numerical value using the source and target units' scales and offsets.
 * All dimension exponents, including AngleExponent, must match.
 */
template<UnitConcept SourceUnit, UnitConcept TargetUnit, ValueConcept Value>
	requires (is_convertible<SourceUnit, TargetUnit>())
	[[nodiscard]]
	constexpr Value
	convert_value (Value const source_value) noexcept
	{
		return (SourceUnit::to_base_unit_floating_point (source_value) - to_floating_point<typename TargetUnit::Offset, Value>()) / to_floating_point<typename TargetUnit::Scale, Value>();
	}

} // namespace detail


/**
 * Helper for convert(q) and to_with_angle_exponent_change<U>(q).
 * All non-angle dimension exponents must match; AngleExponent may change.
 * Apply the scale/offset conversion and the (2 * pi)^n factor described by
 * to_with_angle_exponent_change<U>(). The numerical type Value is preserved.
 */
template<UnitConcept SourceUnit, ValueConcept Value>
	class QuantityConverter
	{
	  public:
		// Ctor
		explicit constexpr
		QuantityConverter (Quantity<SourceUnit, Value> const& quantity):
			_quantity (quantity)
		{ }

		// Conversion operator
		template<UnitConcept TargetUnit>
			requires (is_convertible<SourceUnit, TargetUnit>() || is_convertible_with_angle<SourceUnit, TargetUnit>())
			[[nodiscard]]
			constexpr
			operator Quantity<TargetUnit, Value>() const noexcept
			{
				if constexpr (is_convertible<SourceUnit, TargetUnit>())
					return Quantity<TargetUnit, Value> (detail::convert_value<SourceUnit, TargetUnit> (_quantity.value()));
				else if constexpr (is_convertible_with_angle<SourceUnit, TargetUnit>())
				{
					int other_angle_exp_diff = TargetUnit::AngleExponent - SourceUnit::AngleExponent;
					// Each added angle exponent interprets one cycle as 2 * pi radians;
					// each removed angle exponent applies the inverse factor.
					auto result = (_quantity.to_base_unit().value() - to_floating_point<typename TargetUnit::Offset, Value>()) / to_floating_point<typename TargetUnit::Scale, Value>();

					// Apply (2 * pi)^other_angle_exp_diff with constexpr-compatible loops:
					while (other_angle_exp_diff > 0)
					{
						result *= 2.0 * M_PI;
						--other_angle_exp_diff;
					}

					while (other_angle_exp_diff < 0)
					{
						result /= 2.0 * M_PI;
						++other_angle_exp_diff;
					}

					return Quantity<TargetUnit, Value> (result);
				}
			}

	  private:
		Quantity<SourceUnit, Value> _quantity;
	};


/**
 * Convert to TargetUnit, preserving Value and allowing AngleExponent to change.
 *
 * All other dimension exponents must match. Apply the usual scale/offset
 * conversion, then multiply by (2 * pi)^n, where
 * n = TargetUnit::AngleExponent - SourceUnit::AngleExponent.
 * With n = 0, the result is the same as si::to<TargetUnit>(q).
 *
 * Frequency (Quantity<Hertz>) measures complete cycles per second and has
 * AngleExponent = 0. Rotational speed is represented by AngularVelocity
 * (Quantity<RadianPerSecond>), which measures radians per second and has
 * AngleExponent = 1. For rotation, one cycle is one full turn (2 * pi radians):
 * 1 Hz corresponds to 2 * pi rad/s, while 1 rad/s corresponds to 1 / (2 * pi) Hz.
 * This explicitly chooses a cycles-to-radians interpretation; si::to<U>() and
 * si::value<U>() require AngleExponent to remain unchanged.
 *
 * Example (with si::literals in scope):
 *   si::Frequency const frequency = 1_Hz;
 *   auto const rotational_speed = si::to_with_angle_exponent_change<si::RadianPerSecond> (frequency); // 2 * pi rad/s
 *   auto const restored_frequency = si::to_with_angle_exponent_change<si::Hertz> (rotational_speed); // 1 Hz
 *
 * In contrast, rad/s and rpm both have AngleExponent = 1. Converting between
 * them only changes the scale: si::to<si::RotationPerMinute>(rotational_speed)
 * gives 60 rpm, without needing an angle-exponent change.
 */
template<UnitConcept TargetUnit, ValueConcept Value, UnitConcept SourceUnit>
	requires (is_convertible<SourceUnit, TargetUnit>() || is_convertible_with_angle<SourceUnit, TargetUnit>())
	[[nodiscard]]
	constexpr Quantity<TargetUnit, Value>
	to_with_angle_exponent_change (Quantity<SourceUnit, Value> const& source_quantity)
	{
		return QuantityConverter (source_quantity);
	}


/**
 * Return a converter whose target unit is inferred from the destination Quantity.
 * Use the same compatibility rules and (2 * pi)^n factor as
 * si::to_with_angle_exponent_change<U>(q), preserving the numerical type Value.
 *
 * Example (with si::literals in scope):
 *   si::AngularVelocity const rotational_speed = si::convert (1_Hz); // 2 * pi rad/s
 *   si::Frequency const frequency = si::convert (rotational_speed); // 1 Hz
 */
template<UnitConcept SourceUnit, ValueConcept Value>
	[[nodiscard]]
	constexpr QuantityConverter<typename Quantity<SourceUnit, Value>::Unit, Value>
	convert (Quantity<SourceUnit, Value> const& source_quantity)
	{
		return QuantityConverter (source_quantity);
	}


/**
 * Convert a numerical value using dynamically specified units' scales and offsets.
 * All dimension exponents, including the angle exponent, must match.
 * Degree-to-radian conversions are allowed; Hertz-to-radian-per-second
 * conversions require an angle-exponent change and are rejected.
 *
 * \throw IncompatibleTypes
 *        If any dimension exponent differs, including the angle exponent.
 */
template<ValueConcept Value>
	[[nodiscard]]
	constexpr Value
	convert_value (DynamicUnit const& source_unit, Value const source_value, DynamicUnit const& target_unit)
	{
		// Require all dimension exponents, including the angle exponent, to match:
		if (source_unit.exponents() != target_unit.exponents())
			throw IncompatibleTypes (source_unit, target_unit);

		// TODO support AngleExponent-conversion

		Value const base_unit_value = source_value * source_unit.scale().to_floating_point() + source_unit.offset().to_floating_point();
		Value result = (base_unit_value - target_unit.offset().to_floating_point()) / target_unit.scale().to_floating_point();
		return result;
	}

} // namespace neutrino::si

#endif

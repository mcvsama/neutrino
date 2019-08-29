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

// Standard:
#include <cstddef>

// Local:
#include "unit.h"
#include "standard_unit_traits.h"


namespace si {

std::string
DynamicUnit::symbol() const
{
	auto s = symbols_map().find (*this);

	if (s != symbols_map().end())
		return s->second;
	else
	{
		if (offset().numerator() != 0 || offset().denominator() != 1)
		{
			return "[unit with offset]";
		}

		std::size_t counted_negatives = 0;

		auto count_if_negative = [&counted_negatives] (int exponent) {
			if (exponent < 0)
				counted_negatives += 1;
		};

		count_if_negative (mass_exponent());
		count_if_negative (length_exponent());
		count_if_negative (time_exponent());
		count_if_negative (current_exponent());
		count_if_negative (temperature_exponent());
		count_if_negative (amount_exponent());
		count_if_negative (luminous_intensity_exponent());
		count_if_negative (angle_exponent());

		std::string result;
		// In such order, first positives:
		{
			int inserted = 0;
			add_positive_single_unit_symbol (inserted, result, mass_exponent(), "kg");
			add_positive_single_unit_symbol (inserted, result, length_exponent(), "m");
			add_positive_single_unit_symbol (inserted, result, time_exponent(), "s");
			add_positive_single_unit_symbol (inserted, result, current_exponent(), "A");
			add_positive_single_unit_symbol (inserted, result, temperature_exponent(), "K");
			add_positive_single_unit_symbol (inserted, result, amount_exponent(), "mol");
			add_positive_single_unit_symbol (inserted, result, luminous_intensity_exponent(), "cd");
			add_positive_single_unit_symbol (inserted, result, angle_exponent(), "rad");
		}

		if (counted_negatives > 0)
			result += '/';

		if (counted_negatives > 1)
			result += '(';

		// Then negatives:
		{
			int inserted = 0;
			add_negative_single_unit_symbol (inserted, result, mass_exponent(), "kg");
			add_negative_single_unit_symbol (inserted, result, length_exponent(), "m");
			add_negative_single_unit_symbol (inserted, result, time_exponent(), "s");
			add_negative_single_unit_symbol (inserted, result, current_exponent(), "A");
			add_negative_single_unit_symbol (inserted, result, temperature_exponent(), "K");
			add_negative_single_unit_symbol (inserted, result, amount_exponent(), "mol");
			add_negative_single_unit_symbol (inserted, result, luminous_intensity_exponent(), "cd");
			add_negative_single_unit_symbol (inserted, result, angle_exponent(), "rad");
		}

		if (counted_negatives > 1)
			result += ')';

		if (scale().numerator() != 1 || scale().denominator() != 1)
			result += kDotProductSymbol_utf8 + std::to_string (scale().to_floating_point());

		return result;
	}
}


inline void
DynamicUnit::add_positive_single_unit_symbol (int& inserted, std::string& result, int exponent, const char* symbol)
{
	if (exponent > 0)
		add_single_unit_symbol (inserted, result, exponent, symbol);
}


inline void
DynamicUnit::add_negative_single_unit_symbol (int& inserted, std::string& result, int exponent, const char* symbol)
{
	if (exponent < 0)
		add_single_unit_symbol (inserted, result, -exponent, symbol);
}


inline void
DynamicUnit::add_single_unit_symbol (int& inserted, std::string& result, int exponent, const char* symbol)
{
	using std::to_string;

	if (exponent >= 1 || exponent < 0)
		if (inserted > 0)
			result += kDotProductSymbol_utf8;

	if (exponent != 0)
		inserted += 1;

	if (exponent == 1)
	{
		result += symbol;
	}
	else if (exponent == 2)
	{
		result += symbol;
		result += "²";
	}
	else if (exponent == 3)
	{
		result += symbol;
		result += "³";
	}
	else if (exponent > 1 || exponent < 0)
	{
		result += symbol;
		result += "^";
		result += to_string (exponent);
	}
}

} // namespace si


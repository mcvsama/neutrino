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

#ifndef NEUTRINO__MATH__DEBUG_PRINTS_H__INCLUDED
#define NEUTRINO__MATH__DEBUG_PRINTS_H__INCLUDED

// Neutrino:
#include <neutrino/math/math.h>

// Standard:
#include <cstddef>
#include <iomanip>
#include <ostream>
#include <sstream>


namespace neutrino::debug {

template<class Value, std::size_t Size, class TargetSpace, class SourceSpace>
	std::ostream&
	operator<< (std::ostream& os, math::Vector<Value, Size, TargetSpace, SourceSpace> const& vector)
	{
		for (std::size_t i = 0; i < Size; ++i)
			os << std::showpos << std::fixed << vector[i] << (i != Size - 1 ? " " : "");

		return os;
	}


template<class Value, std::size_t Columns, std::size_t Rows, class TargetSpace, class SourceSpace>
	std::ostream&
	operator<< (std::ostream& os, math::Matrix<Value, Columns, Rows, TargetSpace, SourceSpace> const& matrix)
	{
		for (std::size_t c = 0; c < Columns; ++c)
		{
			for (std::size_t r = 0; r < Rows; ++r)
				os << std::showpos << std::fixed << matrix[c, r] << (r < Rows - 1 ? " " : "");

			if (c < Columns - 1)
				os << ", ";
		}

		return os;
	}


template<class Value, std::size_t Columns, std::size_t Rows, class TargetSpace, class SourceSpace>
	std::string
	to_string (math::Matrix<Value, Columns, Rows, TargetSpace, SourceSpace> const& matrix)
	{
		std::ostringstream ss;
		ss << matrix;
		return ss.str();
	}

} // namespace neutrino::debug

#endif


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

#ifndef NEUTRINO__TEST__TEST_ASSERTS_H__INCLUDED
#define NEUTRINO__TEST__TEST_ASSERTS_H__INCLUDED

// Local:
#include "stdexcept.h"

// Neutrino:
#include <neutrino/math/matrix.h>
#include <neutrino/math/debug_prints.h>
#include <neutrino/types.h>

// Standard:
#include <cmath>
#include <cstddef>


namespace neutrino::test_asserts {
namespace detail {

template<class T1, class T2, class T3>
	inline void
	verify_equal_with_epsilon_scalar (std::string const& test_explanation, T1 const& value1, T2 const& value2, T3 const& epsilon)
	{
		using std::isfinite;

		if (!isfinite (value1) || !isfinite (value2) || value1 - value2 > epsilon || value2 - value1 > epsilon)
		{
			using std::to_string;

			throw TestAssertFailed (test_explanation, "actual value " + to_string (value1) + " not equal to expected " +
									to_string (value2) + " with epsilon " +
									to_string (epsilon) + "; diff=" +
									to_string (value2 - value1));
		}
	}


template<class T1, class T2, class T3>
	inline void
	verify_equal_with_epsilon_vector (std::string const& test_explanation, T1 const& value1, T2 const& value2, T3 const& epsilon)
	{
		using std::isfinite;

		if (!isfinite (abs (value1 - value2)) || abs (value1 - value2) > epsilon)
		{
			using std::to_string;
			using debug::to_string;

			throw TestAssertFailed (test_explanation, "actual value " + to_string (value1) + " not equal to expected " +
									to_string (value2) + " with epsilon " +
									to_string (epsilon) + "; abs diff=" +
									to_string (abs (value2 - value1)));
		}
	}


template<class T1, class T2, class T3>
	inline void
	verify_equal_with_epsilon_matrix (std::string const& test_explanation, T1 const& value1, T2 const& value2, T3 const& epsilon)
	{
		using std::isfinite;

		if (!isfinite (euclidean_norm (value1 - value2)) || euclidean_norm (value1 - value2) > epsilon)
		{
			using std::to_string;
			using debug::to_string;

			throw TestAssertFailed (test_explanation, "actual value " + to_string (value1) + " not equal to expected " +
									to_string (value2) + " with epsilon " +
									to_string (epsilon) + "; euclidean_norm diff=" +
									to_string (euclidean_norm (value2 - value1)));
		}
	}

} // namespace detail

/**
 * Accept any expression without generating 'unused variable' warning.
 * Test expression validity.
 */
template<class T>
	inline void
	verify_compilation (T&&)
	{ }


inline void
verify (std::string const& test_explanation, bool condition)
{
	if (!condition)
		throw TestAssertFailed (test_explanation, "condition failed");
}


template<class T1, class T2, class T3>
	inline void
	verify_equal_with_epsilon (std::string const& test_explanation, T1 const& value1, T2 const& value2, T3 const& epsilon)
	{
		if constexpr (std::is_base_of_v<math::BasicMatrix, T1> && std::is_base_of_v<math::BasicMatrix, T2>)
		{
			if constexpr (value1.is_vector() && value2.is_vector())
				detail::verify_equal_with_epsilon_vector (test_explanation, value1, value2, epsilon);
			else
				detail::verify_equal_with_epsilon_matrix (test_explanation, value1, value2, epsilon);
		}
		else
			detail::verify_equal_with_epsilon_scalar (test_explanation, value1, value2, epsilon);
	}

} // namespace neutrino::test_asserts

#endif


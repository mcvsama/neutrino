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

using std::isfinite;
using std::to_string;
using debug::to_string;


template<class T1, class T2, class T3>
	inline bool
	equal_with_epsilon_scalar (T1 const& value1, T2 const& value2, T3 const& epsilon)
	{
		return isfinite (value1) && isfinite (value2) && value1 - value2 <= epsilon && value2 - value1 <= epsilon;
	}


template<class T1, class T2, class T3>
	inline bool
	equal_with_epsilon_vector (T1 const& value1, T2 const& value2, T3 const& epsilon)
	{
		return isfinite (abs (value1 - value2)) && abs (value1 - value2) <= epsilon;
	}


template<class T1, class T2, class T3>
	inline bool
	equal_with_epsilon_matrix (T1 const& value1, T2 const& value2, T3 const& epsilon)
	{
		return isfinite (euclidean_norm (value1 - value2)) && euclidean_norm (value1 - value2) <= epsilon;
	}


template<class T1, class T2, class T3>
	inline bool
	equal_with_epsilon_quaternion (T1 const& value1, T2 const& value2, T3 const& epsilon)
	{
		return equal_with_epsilon_vector (value1, value2, epsilon);
	}


template<class T1, class T2, class T3>
	inline void
	verify_equal_with_epsilon_scalar (std::string const& test_expectation, T1 const& value1, T2 const& value2, T3 const& epsilon)
	{
		if (!equal_with_epsilon_scalar (value1, value2, epsilon))
		{
			throw TestAssertFailed (test_expectation, "actual value " + to_string (value1) + " not equal to expected " +
									to_string (value2) + " with epsilon " +
									to_string (epsilon) + "; diff=" +
									to_string (value2 - value1));
		}
	}


template<class T1, class T2, class T3>
	inline void
	verify_equal_with_epsilon_vector (std::string const& test_expectation, T1 const& value1, T2 const& value2, T3 const& epsilon)
	{
		if (!equal_with_epsilon_vector (value1, value2, epsilon))
		{
			throw TestAssertFailed (test_expectation, "actual value " + to_string (value1) + " not equal to expected " +
									to_string (value2) + " with epsilon " +
									to_string (epsilon) + "; abs diff=" +
									to_string (abs (value2 - value1)));
		}
	}


template<class T1, class T2, class T3>
	inline void
	verify_equal_with_epsilon_matrix (std::string const& test_expectation, T1 const& value1, T2 const& value2, T3 const& epsilon)
	{
		if (!equal_with_epsilon_matrix (value1, value2, epsilon))
		{
			throw TestAssertFailed (test_expectation, "actual value " + to_string (value1) + " not equal to expected " +
									to_string (value2) + " with epsilon " +
									to_string (epsilon) + "; euclidean_norm diff=" +
									to_string (euclidean_norm (value2 - value1)));
		}
	}


template<class T1, class T2, class T3>
	inline void
	verify_equal_with_epsilon_quaternion (std::string const& test_expectation, T1 const& value1, T2 const& value2, T3 const& epsilon)
	{
		// Treat it like a vector, the function uses abs() anyway on the objects:
		return verify_equal_with_epsilon_vector (test_expectation, value1, value2, epsilon);
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


/**
 * Verifies that a given function throws an exception.
 *
 * This function runs `tested_code()` and ensures that an exception is thrown.
 * If no exception is thrown, it raises a `TestAssertFailed` error.
 *
 * - If `ExpectedException` is `void` (default), it accepts any exception type.
 * - If `ExpectedException` is specified, it verifies that the thrown exception
 *   matches the expected type. If a different exception is thrown, it fails the test.
 *
 * \tparam	ExpectedException The expected exception type. Defaults to `void`, allowing any exception.
 * \param	test_expectation A string describing the expected behavior for debugging.
 * \param	tested_code The function that should throw an exception.
 *
 * \throws	TestAssertFailed if no exception is thrown or an unexpected exception type is caught.
 *
 * \example
 * ```cpp
 * // Any exception is acceptable:
 * verify_throws ("Function should throw", [] { throw std::runtime_error ("error"); });
 *
 * // Expecting a specific exception type:
 * verify_throws<std::runtime_error> ("Expected std::runtime_error", [] { throw std::runtime_error ("error"); });
 *
 * // This test will fail because it throws the wrong type:
 * verify_throws<std::logic_error> ("Should fail because it throws wrong type", [] { throw std::runtime_error ("error"); });
 * ```
 */
template<class ExpectedException = void>
	inline void
	verify_throws (std::string const& test_expectation, std::function<void()> tested_code)
	{
		bool thrown = false;

		if constexpr (std::is_same_v<ExpectedException, void>)
		{
			try {
				tested_code();
			} catch (...) {
				thrown = true;
			}
		}
		else
		{
			try {
				tested_code();
			} catch (ExpectedException&) {
				thrown = true;
				// Ok, that's expected.
			} catch (...) {
				// Unexpected exception.
				throw TestAssertFailed (test_expectation, "thrown unexpected exception");
			}
		}

		if (!thrown)
			throw TestAssertFailed (test_expectation, "not thrown");
	}


inline void
verify (std::string const& test_expectation, bool condition)
{
	if (!condition)
		throw TestAssertFailed (test_expectation, "condition failed");
}


template<class T1, class T2>
	inline void
	verify_equal (std::string const& test_expectation, T1 const& value1, T2 const& value2)
	{
		verify (test_expectation, value1 == value2);
	}


template<class T1, class T2, class T3>
	inline bool
	equal_with_epsilon (T1 const& value1, T2 const& value2, T3 const& epsilon)
	{
		if constexpr (std::is_base_of_v<math::BasicMatrix, T1> && std::is_base_of_v<math::BasicMatrix, T2>)
		{
			if constexpr (value1.is_vector() && value2.is_vector())
				return detail::equal_with_epsilon_vector (value1, value2, epsilon);
			else
				return detail::equal_with_epsilon_matrix (value1, value2, epsilon);
		}
		else if constexpr (std::is_base_of_v<math::BasicQuaternion, T1> && std::is_base_of_v<math::BasicQuaternion, T2>)
			return detail::equal_with_epsilon_quaternion (value1, value2, epsilon);
		else
			return detail::equal_with_epsilon_scalar (value1, value2, epsilon);
	}


template<class T1, class T2, class T3>
	inline void
	verify_equal_with_epsilon (std::string const& test_expectation, T1 const& value1, T2 const& value2, T3 const& epsilon)
	{
		if constexpr (std::is_base_of_v<math::BasicMatrix, T1> && std::is_base_of_v<math::BasicMatrix, T2>)
		{
			if constexpr (value1.is_vector() && value2.is_vector())
				detail::verify_equal_with_epsilon_vector (test_expectation, value1, value2, epsilon);
			else
				detail::verify_equal_with_epsilon_matrix (test_expectation, value1, value2, epsilon);
		}
		else if constexpr (std::is_base_of_v<math::BasicQuaternion, T1> && std::is_base_of_v<math::BasicQuaternion, T2>)
			detail::verify_equal_with_epsilon_quaternion (test_expectation, value1, value2, epsilon);
		else
			detail::verify_equal_with_epsilon_scalar (test_expectation, value1, value2, epsilon);
	}

} // namespace neutrino::test_asserts

#endif


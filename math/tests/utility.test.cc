/* vim:ts=4
 *
 * Copyleft 2026  Michał Gawron
 * Marduk Unix Labs, http://mulabs.org/
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Visit http://www.gnu.org/licenses/gpl-3.0.html for more information on licensing.
 */

// Neutrino:
#include <neutrino/math/utility.h>
#include <neutrino/test/test_asserts.h>
#include <neutrino/test/test_suite.h>

// Standard:
#include <array>
#include <concepts>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <type_traits>


namespace neutrino::test {
namespace {

template<class Integer, class Index>
	concept HasMersenneNumber = requires (Index index) { math::mersenne_number<Integer> (index); };


static TestRegistration tests ([]{
	describe ("Neutrino", "math", "Utility", []{
		it ("computes the 61-bit Mersenne number without floating-point rounding", []{
			constexpr auto expected = std::uint64_t { 2305843009213693951 };
			static_assert (math::mersenne_number<std::uint64_t> (61) == expected);
			static_assert (math::mersenne_prime<std::uint64_t> (61) == expected);
			static_assert (std::is_same_v<decltype (math::mersenne_number<std::uint64_t> (61)), std::uint64_t>);

			test_asserts::verify_equal ("61-bit result is exact", math::mersenne_number<std::uint64_t> (61), expected);
			test_asserts::verify_equal ("legacy name returns the same exact result", math::mersenne_prime<std::uint64_t> (61), expected);
		});

		it ("computes Mersenne numbers including zero, one and composite results", []{
			constexpr std::array<unsigned, 7> expected { 0, 1, 3, 7, 15, 31, 63 };

			for (unsigned index = 0; index < expected.size(); ++index)
				test_asserts::verify_equal ("small Mersenne number", math::mersenne_number<unsigned> (index), expected[index]);

			test_asserts::verify_equal ("a prime exponent does not guarantee a prime result", math::mersenne_number<unsigned> (11), 2047u);
		});

		it ("handles every representable exponent for signed and unsigned integer types", []{
			auto const verify_type = []<std::integral Integer> {
				constexpr auto digits = std::numeric_limits<Integer>::digits;
				constexpr auto maximum = std::numeric_limits<Integer>::max();

				for (int index = 0; index <= digits; ++index)
				{
					// Use a right-shift oracle so the check never forms the unrepresentable 2^digits.
					auto const expected = index == 0 ? Integer (0) : static_cast<Integer> (maximum >> (digits - index));
					test_asserts::verify_equal ("all representable exponents are exact", math::mersenne_number<Integer> (index), expected);
					test_asserts::verify_equal ("legacy alias supports the full range", math::mersenne_prime<Integer> (index), expected);
				}
			};

			verify_type.operator()<bool>();
			verify_type.operator()<std::int8_t>();
			verify_type.operator()<std::uint8_t>();
			verify_type.operator()<std::int16_t>();
			verify_type.operator()<std::uint16_t>();
			verify_type.operator()<std::int32_t>();
			verify_type.operator()<std::uint32_t>();
			verify_type.operator()<std::int64_t>();
			verify_type.operator()<std::uint64_t>();
		});

		it ("rejects negative exponents before shifting", []{
			for (auto const index: { std::int64_t (-1), std::numeric_limits<std::int64_t>::min() })
			{
				test_asserts::verify_throws<std::out_of_range> ("negative exponent is invalid", [index] {
					(void) math::mersenne_number<std::uint64_t> (index);
				});
				test_asserts::verify_throws<std::out_of_range> ("legacy alias rejects negative exponents", [index] {
					(void) math::mersenne_prime<std::uint64_t> (index);
				});
			}
		});

		it ("rejects overflowing exponents before shifting", []{
			auto const verify_type = []<std::integral Integer> {
				for (auto const index: { std::uint64_t (std::numeric_limits<Integer>::digits + 1), std::numeric_limits<std::uint64_t>::max() })
				{
					test_asserts::verify_throws<std::overflow_error> ("exponent exceeds the result type's range", [index] {
						(void) math::mersenne_number<Integer> (index);
					});
					test_asserts::verify_throws<std::overflow_error> ("legacy alias rejects overflowing exponents", [index] {
						(void) math::mersenne_prime<Integer> (index);
					});
				}
			};

			verify_type.operator()<bool>();
			verify_type.operator()<std::int8_t>();
			verify_type.operator()<std::uint8_t>();
			verify_type.operator()<std::int16_t>();
			verify_type.operator()<std::uint16_t>();
			verify_type.operator()<std::int32_t>();
			verify_type.operator()<std::uint32_t>();
			verify_type.operator()<std::int64_t>();
			verify_type.operator()<std::uint64_t>();
		});

		it ("accepts integral exponents and rejects floating-point arguments and result types", []{
			static_assert (HasMersenneNumber<std::uint64_t, std::uint64_t>);
			static_assert (!HasMersenneNumber<std::uint64_t, double>);
			static_assert (!HasMersenneNumber<double, int>);
			static_assert (math::mersenne_number<std::uint64_t> (true) == 1);
			static_assert (math::mersenne_number<std::uint64_t> (char (4)) == 15);
			static_assert (math::mersenne_number<std::uint64_t> (std::uint8_t (64)) == std::numeric_limits<std::uint64_t>::max());
			static_assert (math::mersenne_number<std::uint64_t> (std::int64_t (61)) == 2305843009213693951ULL);
		});
	});
});

} // namespace
} // namespace neutrino::test

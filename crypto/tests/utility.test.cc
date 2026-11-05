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
#include <neutrino/crypto/utility.h>
#include <neutrino/test/test_asserts.h>
#include <neutrino/test/test_suite.h>

// Standard:
#include <cstddef>
#include <limits>
#include <utility>


namespace neutrino::test {
namespace {

static void
verify_integer_wipe (boost::multiprecision::cpp_int& integer)
{
	auto const* storage = integer.backend().limbs();
	auto const capacity = integer.backend().capacity();
	wipe (integer);
	test_asserts::verify ("wiping leaves a valid zero integer", integer == 0);
	test_asserts::verify ("zero has one active limb and no negative sign", integer.backend().size() == 1 && !integer.backend().sign());
	test_asserts::verify ("wiping preserves the cleared allocation", integer.backend().limbs() == storage && integer.backend().capacity() == capacity);

	for (std::size_t i = 0; i < capacity; ++i)
		test_asserts::verify ("all allocated limbs are cleared, including unused capacity", storage[i] == 0);
}


static TestRegistration tests ([]{
	describe ("Neutrino", "crypto", "utility", []{
		it ("securely wipes small and multi-limb integers of either sign", []{
			using Integer = boost::multiprecision::cpp_int;
			auto const limb_bits = std::size_t (std::numeric_limits<boost::multiprecision::limb_type>::digits);

			for (auto const bits: { std::size_t (1), limb_bits - 1, limb_bits, limb_bits + 1, std::size_t (256), std::size_t (2048), std::size_t (4096) })
			{
				Integer const magnitude = (Integer (1) << bits) - 1;

				for (auto const negative: { false, true })
				{
					Integer integer = magnitude;
					if (negative)
						integer = -integer;
					verify_integer_wipe (integer);
				}
			}

			Integer zero { 0 };
			verify_integer_wipe (zero);
			verify_integer_wipe (zero);
			zero += 7;
			test_asserts::verify ("a wiped integer can be reused for arithmetic", zero == 7);
		});

		it ("wipes unused limbs after reducing a large integer to a small value", []{
			using Integer = boost::multiprecision::cpp_int;

			for (auto const value: { 0, 1, -1 })
			{
				Integer integer = (Integer (1) << 4096) - 1;
				integer = value;
				verify_integer_wipe (integer);
			}
		});

		it ("wipes moved-from integers without modifying the moved value", []{
			using Integer = boost::multiprecision::cpp_int;

			for (auto const bits: { 8, 4096 })
			{
				Integer const expected = (Integer (1) << bits) - 1;
				Integer source = expected;
				Integer const destination (std::move (source));
				verify_integer_wipe (source);
				test_asserts::verify ("wiping the source preserves the moved integer", destination == expected);
			}
		});

		it ("rejects zero-sized XOR reduction for empty and nonempty inputs", []{
			for (auto const& source: { Blob {}, Blob { 0x42 } })
			{
				test_asserts::verify_throws<InvalidArgument> ("zero target size is always invalid", [&]{
					[[maybe_unused]] auto const result = xor_reduce (source, 0);
				});
			}
		});

		it ("XORs every byte into the expected output position", []{
			Blob const source { 0x01, 0x02, 0x04, 0x08, 0x10 };
			test_asserts::verify_equal ("reduction includes a partial final group", xor_reduce (source, 2), Blob { 0x15, 0x0a });
			test_asserts::verify_equal ("one-byte output includes every input byte", xor_reduce (source, 1), Blob { 0x1f });
			test_asserts::verify_equal ("matching size preserves the source", xor_reduce (source, 5), source);
		});

		it ("returns a zero-filled result for an empty source and a positive target size", []{
			test_asserts::verify_equal ("empty input preserves the requested output size", xor_reduce (Blob {}, 3), Blob (3, 0));
		});
	});
});

} // namespace
} // namespace neutrino::test

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


namespace neutrino::test {
namespace {

static TestRegistration tests ([]{
	describe ("Neutrino", "crypto", "utility", []{
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

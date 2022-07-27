/* vim:ts=4
 *
 * Copyleft 2008…2013  Michał Gawron
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
#include <neutrino/test/auto_test.h>
#include <neutrino/crypto/hash.h>
#include <neutrino/string.h>

// Standard:
#include <cstddef>


namespace neutrino::test {
namespace {

AutoTest t1 ("neutrino::calculate_hash()", []{
	auto const hash_sha2_256 = calculate_hash<Hash::SHA2_256> (value_to_blob ("test hash"));
	test_asserts::verify ("computed hash SHA2-256 is correct", to_hex_string (hash_sha2_256) == "54a6483b8aca55c9df2a35baf71d9965ddfd623468d81d51229bd5eb7d1e1c1b");

	auto const hash_sha3_512 = calculate_hash<Hash::SHA3_512> (value_to_blob ("test hash"));
	test_asserts::verify ("computed hash SHA3-512 is correct", to_hex_string (hash_sha3_512) == "3a035bfe9888833464b0ddd897de9556f5e2dda44938d73ac64f4eea1ecdf6e24f7f2023dce4096377594f7ee93c26857d16ac8f7fd44671ef60c471e623bbb9");
});

} // namespace
} // namespace neutrino::test


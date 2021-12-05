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

// Standard:
#include <cstddef>

// Neutrino:
#include <neutrino/test/auto_test.h>
#include <neutrino/crypto/hkdf.h>
#include <neutrino/string.h>


namespace neutrino::test {
namespace {

AutoTest t1 ("neutrino::calculate_hkdf()", []{
	auto const output_key = calculate_hkdf<Hash::SHA2_256> ({
		.salt = value_to_blob ("salt"),
		.key_material = value_to_blob ("key"),
		.info = value_to_blob ("info"),
		.result_length = 64,
	});

	test_asserts::verify ("computed output key is correct", to_hex_string (output_key) == "9ca0d662557439e3b83365f2da4626d35da195c6d9d1779f09838cf9e408966ece99106e2585ace6f083fa1acd5192f430fd55bc741184dbb822fc04c3ebfd3d");
});

} // namespace
} // namespace neutrino::test


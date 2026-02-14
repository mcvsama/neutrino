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
#include <neutrino/crypto/hmac.h>
#include <neutrino/string.h>

// Standard:
#include <cstddef>


namespace neutrino::test {
namespace {

AutoTest t1 ("neutrino::compute_hmac()", []{
	auto const hmac = compute_hmac<Hash::SHA2_256> ({
		.data = to_blob ("data"),
		.key = to_blob ("key"),
	});

	test_asserts::verify ("computed HMAC is correct", to_hex_string (hmac) == "5031fe3d989c6d1537a013fa6e739da23463fdaec3b70137d828e36ace221bd0");
});

} // namespace
} // namespace neutrino::test

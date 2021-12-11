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
#include <neutrino/crypto/diffie_hellman_exchange.h>


namespace neutrino::test {
namespace {

AutoTest t1 ("neutrino::DiffieHellmanExchange", []{
	boost::random::random_device rnd;

	DiffieHellmanExchange ex1 (rnd);
	DiffieHellmanExchange ex2 (rnd);

	auto const pub1_int = ex1.exchange_integer();
	auto const pub2_int = ex2.exchange_integer();

	test_asserts::verify ("public value 1 is not 0", pub1_int != 0);
	test_asserts::verify ("public value 2 is not 0", pub2_int != 0);

	auto const pub1 = ex1.exchange_blob();
	auto const pub2 = ex2.exchange_blob();

	test_asserts::verify ("public value 1 is not longer than expected", pub1.size() <= ex1.max_blob_size());
	test_asserts::verify ("public value 2 is not longer than expected", pub2.size() <= ex2.max_blob_size());

	auto const key1_int = ex1.calculate_key_with_weak_bits (pub1_int);
	auto const key2_int = ex2.calculate_key_with_weak_bits (pub2_int);

	auto const key1 = ex1.calculate_key_with_weak_bits (pub2);
	auto const key2 = ex2.calculate_key_with_weak_bits (pub1);

	test_asserts::verify ("key 1 is not 0", key1_int != 0);
	test_asserts::verify ("key 2 is not 0", key2_int != 0);
	test_asserts::verify ("computed key is equal on both sides", key1 == key2);
});

} // namespace
} // namespace neutrino::test


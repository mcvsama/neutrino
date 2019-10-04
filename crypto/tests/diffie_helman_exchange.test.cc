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
#include <neutrino/crypto/diffie_helman_exchange.h>


namespace neutrino::test {
namespace {

AutoTest t1 ("neutrino::DiffieHelmanExchange", []{
	boost::random::random_device rnd;

	DiffieHelmanExchange ex1 (rnd);
	DiffieHelmanExchange ex2 (rnd);

	auto pub1_int = ex1.exchange_integer();
	auto pub2_int = ex2.exchange_integer();

	test_asserts::verify ("public value 1 is not 0", pub1_int != 0);
	test_asserts::verify ("public value 2 is not 0", pub2_int != 0);

	auto pub1 = ex1.exchange_blob();
	auto pub2 = ex2.exchange_blob();

	auto key1_int = ex1.calculate_key (pub1_int);
	auto key2_int = ex2.calculate_key (pub2_int);

	auto key1 = ex1.calculate_key (pub2);
	auto key2 = ex2.calculate_key (pub1);

	test_asserts::verify ("key 1 is not 0", key1_int != 0);
	test_asserts::verify ("key 2 is not 0", key2_int != 0);
	test_asserts::verify ("computed key is equal on both sides", key1 == key2);
});

} // namespace
} // namespace neutrino::test


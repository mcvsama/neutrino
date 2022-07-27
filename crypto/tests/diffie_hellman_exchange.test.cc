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
#include <neutrino/crypto/diffie_hellman_exchange.h>

// Standard:
#include <cstddef>


namespace neutrino::test {
namespace {

AutoTest t1 ("neutrino::DiffieHellmanExchange", []{
	boost::random::random_device rnd;

	DiffieHellmanExchange ex1 (rnd);
	DiffieHellmanExchange ex2 (rnd);

	auto const pub1 = ex1.generate_exchange_blob();
	auto const pub2 = ex2.generate_exchange_blob();

	test_asserts::verify ("public value 1 is not 0", DiffieHellmanExchange::to_integer (pub1) != 0);
	test_asserts::verify ("public value 2 is not 0", DiffieHellmanExchange::to_integer (pub2) != 0);

	test_asserts::verify ("public value 1 is longer than expected", pub1.size() <= ex1.max_blob_size());
	test_asserts::verify ("public value 2 is longer than expected", pub2.size() <= ex2.max_blob_size());

	auto const key1 = ex1.calculate_key_with_weak_bits (pub2);
	auto const key2 = ex2.calculate_key_with_weak_bits (pub1);

	test_asserts::verify ("key 1 is not 0", DiffieHellmanExchange::to_integer (key1) != 0);
	test_asserts::verify ("key 2 is not 0", DiffieHellmanExchange::to_integer (key2) != 0);
	test_asserts::verify ("computed key is equal on both sides", key1 == key2);
});


AutoTest t2 ("neutrino::DiffieHellmanExchange ensures that generate_exchange_blob() is called first", []{
	bool thrown = false;

	try {
		boost::random::random_device rnd;
		DiffieHellmanExchange ex1 (rnd);
		[[maybe_unused]] auto const key = ex1.calculate_key_with_weak_bits (Blob { 0x00, 0x01, 0x02 });
	}
	catch (std::runtime_error&)
	{
		thrown = true;
	}

	test_asserts::verify ("DiffieHellmanExchange throws if generate_exchange_blob() is not called first", thrown);
});

} // namespace
} // namespace neutrino::test


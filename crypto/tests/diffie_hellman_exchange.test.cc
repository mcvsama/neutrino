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
#include <neutrino/test/test_asserts.h>
#include <neutrino/test/test_suite.h>
#include <neutrino/crypto/diffie_hellman_exchange.h>

// Standard:
#include <cstddef>
#include <random>


namespace neutrino::test {
namespace {

TestRegistration tests ([]{
	describe ("Neutrino", "crypto", "DiffieHellmanExchange", []{
		it ("computes matching shared key", []{
			auto rnd = std::random_device ("hw");

			DiffieHellmanExchange ex1 (rnd);
			DiffieHellmanExchange ex2 (rnd);

			auto const pub1 = ex1.public_share();
			auto const pub2 = ex2.public_share();

			test_asserts::verify ("public value 1 is not 0", DiffieHellmanExchange::to_integer (pub1) != 0);
			test_asserts::verify ("public value 2 is not 0", DiffieHellmanExchange::to_integer (pub2) != 0);

			test_asserts::verify ("public value 1 is not longer than expected",
								  pub1.size() <= DiffieHellmanExchange::max_blob_size (RFC3526_Group14<DiffieHellmanExchange::Integer>));
			test_asserts::verify ("public value 2 is not longer than expected",
								  pub2.size() <= DiffieHellmanExchange::max_blob_size (RFC3526_Group14<DiffieHellmanExchange::Integer>));

			auto const key1 = ex1.compute_key_with_weak_bits (pub2);
			auto const key2 = ex2.compute_key_with_weak_bits (pub1);

			test_asserts::verify ("key 1 is not 0", DiffieHellmanExchange::to_integer (key1) != 0);
			test_asserts::verify ("key 2 is not 0", DiffieHellmanExchange::to_integer (key2) != 0);
			test_asserts::verify ("computed key is equal on both sides", key1 == key2);
		});

		it ("rejects invalid public shares", []{
			auto rnd = std::random_device ("hw");
			DiffieHellmanExchange exchange (rnd);

			test_asserts::verify_throws<InvalidArgument> ("public share 0 should be rejected", [&]{
				[[maybe_unused]] auto const key = exchange.compute_key_with_weak_bits (Blob());
			});

			test_asserts::verify_throws<InvalidArgument> ("public share 1 should be rejected", [&]{
				[[maybe_unused]] auto const key = exchange.compute_key_with_weak_bits (
					DiffieHellmanExchange::to_blob (1, DiffieHellmanExchange::max_blob_size (RFC3526_Group14<DiffieHellmanExchange::Integer>))
				);
			});

			auto const invalid_high_share = DiffieHellmanExchange::to_blob (
				RFC3526_Group14<DiffieHellmanExchange::Integer>.prime - 1,
				DiffieHellmanExchange::max_blob_size (RFC3526_Group14<DiffieHellmanExchange::Integer>)
			);

			test_asserts::verify_throws<InvalidArgument> ("public share p - 1 should be rejected", [&]{
				[[maybe_unused]] auto const key = exchange.compute_key_with_weak_bits (invalid_high_share);
			});
		});
	});
});

} // namespace
} // namespace neutrino::test

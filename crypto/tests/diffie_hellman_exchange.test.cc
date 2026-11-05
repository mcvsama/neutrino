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
#include <neutrino/crypto/diffie_hellman_exchange.h>
#include <neutrino/test/test_asserts.h>
#include <neutrino/test/test_suite.h>

// Standard:
#include <cstddef>
#include <random>


namespace neutrino::test {
namespace {

using ModpDiffieHellmanExchange = DiffieHellmanExchange<ModpScheme>;
using EllipticCurveDiffieHellmanExchange = DiffieHellmanExchange<EllipticCurveScheme>;


TestRegistration tests ([]{
	describe ("Neutrino", "crypto", "DiffieHellmanExchange", []{
		it ("decodes empty MODP integer encodings as zero", []{
			Blob const empty;
			Blob const bytes { 0x01, 0x02 };

			for (auto const input: { BlobView {}, BlobView (empty), BlobView (bytes).substr (bytes.size()) })
				test_asserts::verify ("empty encodings produce zero without importing an empty range", ModpScheme::to_integer (input) == 0);

			test_asserts::verify ("nonempty encodings retain big-endian interpretation", ModpScheme::to_integer (bytes) == 258);
		});

		it ("computes matching shared key", []{
			auto rnd = std::random_device ("hw");

			ModpDiffieHellmanExchange ex1 (rnd);
			ModpDiffieHellmanExchange ex2 (rnd);

			auto const pub1 = ex1.public_share();
			auto const pub2 = ex2.public_share();

			test_asserts::verify ("public share 1 is not empty", !pub1.empty());
			test_asserts::verify ("public share 2 is not empty", !pub2.empty());

			test_asserts::verify ("public value 1 is not longer than expected",
								  pub1.size() <= ex1.max_blob_size());
			test_asserts::verify ("public value 2 is not longer than expected",
								  pub2.size() <= ex2.max_blob_size());

			auto const key1 = ex1.compute_key_with_weak_bits (pub2);
			auto const key2 = ex2.compute_key_with_weak_bits (pub1);

			test_asserts::verify ("key 1 is not 0", ModpScheme::to_integer (key1) != 0);
			test_asserts::verify ("key 2 is not 0", ModpScheme::to_integer (key2) != 0);
			test_asserts::verify ("computed key is equal on both sides", key1 == key2);
		});

		it ("computes matching shared key for elliptic-curve exchange", []{
			auto rnd = std::random_device ("hw");
			auto const scheme = EllipticCurveScheme();

			EllipticCurveDiffieHellmanExchange ex1 (rnd, scheme);
			EllipticCurveDiffieHellmanExchange ex2 (rnd, scheme);

			auto const pub1 = ex1.public_share();
			auto const pub2 = ex2.public_share();

			test_asserts::verify ("public share 1 is not empty", !pub1.empty());
			test_asserts::verify ("public share 2 is not empty", !pub2.empty());
			test_asserts::verify ("public share 1 is not longer than expected", pub1.size() <= scheme.max_blob_size());
			test_asserts::verify ("public share 2 is not longer than expected", pub2.size() <= scheme.max_blob_size());

			auto const key1 = ex1.compute_key_with_weak_bits (pub2);
			auto const key2 = ex2.compute_key_with_weak_bits (pub1);

			test_asserts::verify ("key 1 is not 0", ModpScheme::to_integer (key1) != 0);
			test_asserts::verify ("key 2 is not 0", ModpScheme::to_integer (key2) != 0);
			test_asserts::verify ("computed key is equal on both sides", key1 == key2);
		});

		it ("rejects invalid MODP public shares", []{
			auto rnd = std::random_device ("hw");
			ModpDiffieHellmanExchange exchange (rnd);

			Blob const empty;
			Blob const zero { 0x00 };

			for (auto const invalid: { BlobView {}, BlobView (empty), BlobView (zero) })
			{
				test_asserts::verify_throws<InvalidArgument> ("empty or zero public shares should be rejected", [&]{
					[[maybe_unused]] auto const key = exchange.compute_key_with_weak_bits (invalid);
				});
			}

			test_asserts::verify_throws<InvalidArgument> ("public share 1 should be rejected", [&]{
				[[maybe_unused]] auto const key = exchange.compute_key_with_weak_bits (
					ModpScheme::to_blob (1, exchange.max_blob_size())
				);
			});

			auto const invalid_high_share = ModpScheme::to_blob (
				RFC3526_Group14<ModpScheme::Integer>.prime - 1,
				exchange.max_blob_size()
			);

			test_asserts::verify_throws<InvalidArgument> ("public share p - 1 should be rejected", [&]{
				[[maybe_unused]] auto const key = exchange.compute_key_with_weak_bits (invalid_high_share);
			});
		});

		it ("rejects invalid elliptic-curve public shares", []{
			auto rnd = std::random_device ("hw");
			auto const scheme = EllipticCurveScheme();
			EllipticCurveDiffieHellmanExchange exchange (rnd, scheme);

			test_asserts::verify_throws<InvalidArgument> ("empty elliptic-curve public share should be rejected", [&]{
				[[maybe_unused]] auto const key = exchange.compute_key_with_weak_bits (Blob());
			});

			test_asserts::verify_throws<InvalidArgument> ("truncated elliptic-curve public share should be rejected", [&]{
				auto invalid_public_share = Blob (exchange.public_share());
				invalid_public_share.resize (invalid_public_share.size() - 1);
				[[maybe_unused]] auto const key = exchange.compute_key_with_weak_bits (invalid_public_share);
			});
		});
	});
});

} // namespace
} // namespace neutrino::test

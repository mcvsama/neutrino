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

		it ("reports MODP blob sizes across byte boundaries", []{
			struct TestCase
			{
				unsigned int prime;
				std::size_t bits;
				std::size_t expected_bytes;
			};

			TestCase const cases[] {
				{ 127, 7, 1 },
				{ 251, 8, 1 },
				{ 257, 9, 2 },
				{ 263, 9, 2 },
				{ 65521, 16, 2 },
				{ 65537, 17, 3 },
			};

			for (auto const& [prime, bits, expected_bytes]: cases)
			{
				ModpScheme const scheme ({ .bits = bits, .generator = 2, .prime = prime });
				test_asserts::verify_equal ("blob size includes the byte containing the highest set bit", scheme.max_blob_size(), expected_bytes);
			}
		});

		it ("fits nine-bit MODP shares and shared keys within the advertised blob size", []{
			// 263 is a safe prime; 2^8 mod 263 = 256 needs two bytes.
			ModpScheme const scheme ({ .bits = 9, .generator = 2, .prime = 263 });
			auto const public_share = ModpScheme::to_blob (
				ModpScheme::compute_public_or_shared_value (2, 263, 8),
				scheme.max_blob_size()
			);
			auto const key1 = scheme.mix (Blob { 8 }, Blob { 2 });
			auto const key2 = scheme.mix (Blob { 1 }, public_share);

			test_asserts::verify_equal ("the public share retains its two-byte big-endian encoding", public_share, Blob { 0x01, 0x00 });
			test_asserts::verify_equal ("both peers compute the expected shared key", key1, public_share);
			test_asserts::verify_equal ("both peers compute matching keys", key1, key2);
			test_asserts::verify ("the public share fits the advertised limit", public_share.size() <= scheme.max_blob_size());
			test_asserts::verify ("the shared key fits the advertised limit", key1.size() <= scheme.max_blob_size());
		});

		it ("preserves modular exponentiation results", []{
			using Integer = ModpScheme::Integer;
			Integer const exponents[] { 0, 1, 255, 256, Integer (1) << 64, (Integer (1) << 129) - 1 };

			// Exercise zero, single-bit and dense exponents across byte/limb boundaries.
			for (auto const& exponent: exponents)
			{
				for (auto const base: { -47, 0, 1, 2, 22, 23, 47 })
				{
					Integer expected = powm (Integer (base), exponent, Integer (23));

					if (expected < 0)
						expected += 23;

					test_asserts::verify ("constant-time exponentiation agrees with the mathematical result",
										  ModpScheme::compute_public_or_shared_value (base, 23, exponent) == expected);
				}
			}

			for (auto const& group: { RFC3526_Group14<Integer>, RFC3526_Group15<Integer> })
			{
				Integer const exponent = (Integer (1) << (group.bits - 1)) + 257;
				Integer const expected = powm (group.generator, exponent, group.prime);
				test_asserts::verify ("RFC3526 public shares retain their value",
									  ModpScheme::compute_public_or_shared_value (group.generator, group.prime, exponent) == expected);
			}

			ModpScheme const small_group ({ .bits = 5, .generator = 2, .prime = 23 });
			test_asserts::verify ("a known shared secret retains its big-endian encoding",
								  small_group.mix (Blob { 6 }, Blob { 13 }) == Blob { 6 });
		});

		it ("rejects unsupported modular exponentiation parameters", []{
			for (auto const modulus: { -23, 0, 1, 2, 24 })
			{
				test_asserts::verify_throws<InvalidArgument> ("invalid moduli do not select a variable-time fallback", [&]{
					[[maybe_unused]] auto const result = ModpScheme::compute_public_or_shared_value (2, modulus, 6);
				});
			}

			test_asserts::verify_throws<InvalidArgument> ("negative secret exponents are rejected", []{
				[[maybe_unused]] auto const result = ModpScheme::compute_public_or_shared_value (2, 23, -1);
			});
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

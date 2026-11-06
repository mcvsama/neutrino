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

// Crypto++:
#include <cryptopp/eccrypto.h>
#include <cryptopp/integer.h>
#include <cryptopp/oids.h>

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

		it ("preserves MODP integer encodings across limb boundaries", []{
			for (auto const length: { 1u, 7u, 8u, 9u, 15u, 16u, 17u, 31u, 32u, 33u, 256u })
			{
				Blob bytes (length, 0);

				for (size_t i = 0; i < bytes.size(); ++i)
					bytes[i] = static_cast<Blob::value_type> (17 * i + 1);

				ModpScheme::Integer expected;
				boost::multiprecision::import_bits (expected, bytes.begin(), bytes.end());
				auto const value = ModpScheme::to_integer (bytes);
				test_asserts::verify_equal ("decoding agrees with Boost import_bits", value, expected);

				for (auto const reserve: { size_t (0), size_t (512) })
				{
					test_asserts::verify_equal ("encoding preserves the minimal big-endian representation", ModpScheme::to_blob (value, reserve), bytes);
					test_asserts::verify_equal ("encoding retains export_bits' magnitude semantics", ModpScheme::to_blob (-value, reserve), bytes);
				}

				test_asserts::verify_equal ("leading zero padding does not affect decoding", ModpScheme::to_integer (Blob (19, 0) + bytes), value);
				test_asserts::verify ("all-zero limbs decode as zero", ModpScheme::to_integer (Blob (length, 0)) == 0);
			}

			test_asserts::verify_equal ("zero retains a single-byte encoding", ModpScheme::to_blob (0, 0), Blob { 0 });
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

		it ("generates valid MODP exchanges within subgroup and bit-length limits", []{
			struct TestCase
			{
				std::size_t bits;
				unsigned int max_secret;
			};

			TestCase const cases[] {
				{ 1, 1 },
				{ 3, 7 },
				{ 4, 10 },
				{ 5, 10 },
			};
			auto rnd = std::random_device ("hw");

			// For p = 23 and g = 2, q = 11. Old generation could choose 11 or 22,
			// yielding public share 1, which the peer then rejected.
			for (auto const& [bits, max_secret]: cases)
			{
				ModpScheme const scheme ({ .bits = bits, .generator = 2, .prime = 23 });

				for (std::size_t i = 0; i < 32; ++i)
				{
					auto const values1 = scheme.generate_exchange (rnd);
					auto const values2 = scheme.generate_exchange (rnd);

					for (auto const* values: { &values1, &values2 })
					{
						auto const secret = ModpScheme::to_integer (*values->private_value);
						auto const public_share = ModpScheme::to_integer (values->public_share);
						test_asserts::verify ("the private exponent respects both bounds", secret >= 1 && secret <= max_secret);
						test_asserts::verify ("the public share is nontrivial", public_share > 1 && public_share < 22);
					}

					auto const key1 = scheme.mix (*values1.private_value, values2.public_share);
					auto const key2 = scheme.mix (*values2.private_value, values1.public_share);
					test_asserts::verify_equal ("generated shares pass validation and produce matching keys", key1, key2);
				}
			}
		});

		it ("rejects empty MODP private-exponent intervals", []{
			auto rnd = std::random_device ("hw");
			ModpScheme const zero_bits ({ .bits = 0, .generator = 2, .prime = 23 });

			test_asserts::verify_throws<InvalidArgument> ("a zero bit limit provides no positive exponent", [&]{
				[[maybe_unused]] auto const values = zero_bits.generate_exchange (rnd);
			});

			for (auto const prime: { -23, 0, 1, 2, 3, 4 })
			{
				ModpScheme const scheme ({ .bits = 5, .generator = 2, .prime = prime });
				test_asserts::verify_throws<InvalidArgument> ("a subgroup order below two provides no positive exponent", [&]{
					[[maybe_unused]] auto const values = scheme.generate_exchange (rnd);
				});
			}
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

		it ("rejects invalid MODP private exponents", []{
			ModpScheme const scheme ({ .bits = 5, .generator = 2, .prime = 23 });
			Blob const invalid_values[] { Blob {}, Blob { 0 }, Blob { 0, 0 }, Blob { 11 }, Blob { 12 }, Blob { 22 }, Blob { 255 }, Blob { 1, 0 } };

			for (auto const& private_value: invalid_values)
			{
				test_asserts::verify_throws<InvalidArgument> ("zero and out-of-range exponents cannot produce a shared key", [&]{
					[[maybe_unused]] auto const key = scheme.mix (private_value, Blob { 2 });
				});
			}

			test_asserts::verify_throws<InvalidArgument> ("a null empty private-key view is rejected", [&]{
				[[maybe_unused]] auto const key = scheme.mix (BlobView {}, Blob { 2 });
			});

			ModpScheme const large_group;
			ModpScheme::Integer const order = (large_group.group().prime - 1) / 2;
			ModpScheme::Integer const invalid_exponents[] { order, order + 1, 2 * order };

			for (auto const& exponent: invalid_exponents)
			{
				test_asserts::verify_throws<InvalidArgument> ("full-width RFC3526 exponents must be below the subgroup order", [&]{
					[[maybe_unused]] auto const key = large_group.mix (ModpScheme::to_blob (exponent, large_group.max_blob_size()), Blob { 2 });
				});
			}
		});

		it ("accepts boundary MODP private exponents", []{
			ModpScheme const scheme ({ .bits = 5, .generator = 2, .prime = 23 });
			test_asserts::verify_equal ("the smallest private exponent remains valid", scheme.mix (Blob { 1 }, Blob { 2 }), Blob { 2 });
			test_asserts::verify_equal ("q - 1 remains valid", scheme.mix (Blob { 10 }, Blob { 2 }), Blob { 12 });
			test_asserts::verify_equal ("the peer computes the same boundary shared key", scheme.mix (Blob { 1 }, Blob { 12 }), Blob { 12 });

			ModpScheme const padded_group ({ .bits = 9, .generator = 2, .prime = 263 });
			test_asserts::verify_equal ("leading zero padding does not change a valid exponent", padded_group.mix (Blob { 0, 1 }, Blob { 2 }), Blob { 2 });

			ModpScheme const large_group;
			ModpScheme::Integer const order = (large_group.group().prime - 1) / 2;
			auto const key = large_group.mix (ModpScheme::to_blob (order - 1, large_group.max_blob_size()), Blob { 2 });
			// Since 2 has order q, 2^(q - 1) is the inverse of 2 modulo p.
			test_asserts::verify ("the full-width upper boundary produces the expected shared key",
								  ModpScheme::to_integer (key) == (large_group.group().prime + 1) / 2);
		});

		it ("enforces the MODP private-exponent bit limit when mixing", []{
			ModpScheme const short_group ({ .bits = 3, .generator = 2, .prime = 23 });
			test_asserts::verify_equal ("2^bits - 1 remains valid", short_group.mix (Blob { 7 }, Blob { 2 }), Blob { 13 });

			for (auto const& private_value: { Blob { 8 }, Blob { 10 } })
			{
				test_asserts::verify_throws<InvalidArgument> ("exponents below q must still respect the bit limit", [&]{
					[[maybe_unused]] auto const key = short_group.mix (private_value, Blob { 2 });
				});
			}

			ModpScheme const zero_bits ({ .bits = 0, .generator = 2, .prime = 23 });
			test_asserts::verify_throws<InvalidArgument> ("an empty private-exponent interval is rejected", [&]{
				[[maybe_unused]] auto const key = zero_bits.mix (Blob { 1 }, Blob { 2 });
			});
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

		it ("rejects invalid elliptic-curve private scalars", []{
			auto const scheme = EllipticCurveScheme();
			auto const domain = CryptoPP::ECDH<CryptoPP::ECP>::Domain (CryptoPP::ASN1::secp256r1());
			auto const& parameters = domain.GetGroupParameters();
			auto const& order = parameters.GetSubgroupOrder();
			auto const private_length = std::size_t (domain.PrivateKeyLength());
			Blob public_share (domain.PublicKeyLength(), 0);
			parameters.EncodeElement (true, domain.GetGenerator(), public_share.data());

			CryptoPP::Integer const invalid_scalars[] {
				CryptoPP::Integer::Zero(),
				order,
				order + 1,
				(CryptoPP::Integer::One() << (8 * private_length)) - 1,
			};

			for (auto const& scalar: invalid_scalars)
			{
				Blob private_value (private_length, 0);
				scalar.Encode (private_value.data(), private_value.size());
				test_asserts::verify_throws<InvalidArgument> ("invalid scalars are rejected before Crypto++ key agreement", [&]{
					[[maybe_unused]] auto const key = scheme.mix (private_value, public_share);
				});
			}

			for (auto const length: { std::size_t (0), private_length - 1, private_length + 1 })
			{
				test_asserts::verify_throws<InvalidArgument> ("private scalars must use the fixed-length encoding", [&]{
					[[maybe_unused]] auto const key = scheme.mix (Blob (length, 0), public_share);
				});
			}

			test_asserts::verify_throws<InvalidArgument> ("a null empty private-key view is rejected", [&]{
				[[maybe_unused]] auto const key = scheme.mix (BlobView {}, public_share);
			});
		});

		it ("accepts boundary elliptic-curve private scalars", []{
			auto const scheme = EllipticCurveScheme();
			auto const domain = CryptoPP::ECDH<CryptoPP::ECP>::Domain (CryptoPP::ASN1::secp256r1());
			auto const& parameters = domain.GetGroupParameters();
			Blob public_share (domain.PublicKeyLength(), 0);
			parameters.EncodeElement (true, domain.GetGenerator(), public_share.data());

			// Multiplication by 1 or n - 1 gives G or -G, with the same x coordinate.
			Blob expected (domain.AgreedValueLength(), 0);
			domain.GetGenerator().x.Encode (expected.data(), expected.size());
			CryptoPP::Integer const valid_scalars[] { CryptoPP::Integer::One(), parameters.GetSubgroupOrder() - 1 };

			for (auto const& scalar: valid_scalars)
			{
				Blob private_value (domain.PrivateKeyLength(), 0);
				scalar.Encode (private_value.data(), private_value.size());
				test_asserts::verify_equal ("both boundary scalars produce the expected shared secret", scheme.mix (private_value, public_share), expected);
			}
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

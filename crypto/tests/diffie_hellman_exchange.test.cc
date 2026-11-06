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
#include <cryptopp/osrng.h>

// Standard:
#include <cstddef>
#include <limits>
#include <random>


namespace neutrino::test {
namespace {

using ModpDiffieHellmanExchange = DiffieHellmanExchange<ModpScheme>;
using EllipticCurveDiffieHellmanExchange = DiffieHellmanExchange<EllipticCurveScheme>;


static Blob
p256_integer_bytes (CryptoPP::Integer const& value)
{
	Blob result (32, 0);
	value.Encode (result.data(), result.size());
	return result;
}


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

		it ("bounds MODP generation by the subgroup for very large bit limits", []{
			auto rnd = std::random_device ("hw");
			auto const verify = [&] (std::size_t const bits) {
				ModpScheme const scheme ({ .bits = bits, .generator = 2, .prime = 23 });
				auto const values1 = scheme.generate_exchange (rnd);
				auto const values2 = scheme.generate_exchange (rnd);

				for (auto const* values: { &values1, &values2 })
				{
					auto const secret = ModpScheme::to_integer (*values->private_value);
					test_asserts::verify ("large bit limits still respect q - 1", secret >= 1 && secret <= 10);
				}

				test_asserts::verify_equal ("large bit limits allow both peers to derive the same key",
											scheme.mix (*values1.private_value, values2.public_share),
											scheme.mix (*values2.private_value, values1.public_share));
			};

			if constexpr (std::numeric_limits<std::size_t>::digits > std::numeric_limits<unsigned int>::digits)
				verify (std::size_t (std::numeric_limits<unsigned int>::max()) + 1);

			verify (std::numeric_limits<std::size_t>::max());
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

		it ("bounds MODP input encodings by the group byte size", []{
			ModpScheme const schemes[] {
				ModpScheme ({ .bits = 5, .generator = 2, .prime = 23 }),
				ModpScheme ({ .bits = 9, .generator = 2, .prime = 263 }),
				ModpScheme(),
			};

			for (auto const& scheme: schemes)
			{
				auto const max_size = scheme.max_blob_size();
				Blob private_value (max_size, 0);
				Blob public_share (max_size, 0);
				private_value.back() = 1;
				public_share.back() = 2;
				test_asserts::verify_equal ("zero padding up to the group limit remains valid", scheme.mix (private_value, public_share), Blob { 2 });
				test_asserts::verify_equal ("minimal encodings remain valid", scheme.mix (Blob { 1 }, Blob { 2 }), Blob { 2 });

				for (auto const length: { max_size + 1, std::size_t (1024 * 1024) })
				{
					// Numerically valid values must still be rejected if their encodings are oversized.
					Blob oversized (length, 0);
					oversized.back() = 2;
					test_asserts::verify_throws<InvalidArgument> ("oversized public shares are rejected even with leading zero padding", [&]{
						[[maybe_unused]] auto const key = scheme.mix (Blob { 1 }, oversized);
					});

					oversized.back() = 1;
					test_asserts::verify_throws<InvalidArgument> ("oversized private exponents are rejected even with leading zero padding", [&]{
						[[maybe_unused]] auto const key = scheme.mix (oversized, Blob { 2 });
					});
				}
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
				test_asserts::verify_throws<InvalidArgument> ("invalid scalars are rejected before point multiplication", [&]{
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

		it ("matches the P-256 ECDH vector from RFC 5903 section 8.1", []{
			// https://www.rfc-editor.org/rfc/rfc5903.html#section-8.1
			auto const scheme = EllipticCurveScheme();
			auto const initiator_private = p256_integer_bytes (CryptoPP::Integer ("C88F01F510D9AC3F70A292DAA2316DE544E9AAB8AFE84049C62A9C57862D1433h"));
			auto const responder_private = p256_integer_bytes (CryptoPP::Integer ("C6EF9C5D78AE012A011164ACB397CE2088685D8F06BF9BE0B283AB46476BEE53h"));
			auto const initiator_public = Blob { 0x04 }
				+ p256_integer_bytes (CryptoPP::Integer ("DAD0B65394221CF9B051E1FECA5787D098DFE637FC90B9EF945D0C3772581180h"))
				+ p256_integer_bytes (CryptoPP::Integer ("5271A0461CDB8252D61F1C456FA3E59AB1F45B33ACCF5F58389E0577B8990BB3h"));
			auto const responder_public = Blob { 0x04 }
				+ p256_integer_bytes (CryptoPP::Integer ("D12DFB5289C8D4F81208B70270398C342296970A0BCCB74C736FC7554494BF63h"))
				+ p256_integer_bytes (CryptoPP::Integer ("56FBF3CA366CC23E8157854C13C58D6AAC23F046ADA30F8353E74F33039872ABh"));
			auto const expected = p256_integer_bytes (CryptoPP::Integer ("D6840F6B42F6EDAFD13116E0E12565202FEF8E9ECE7DCE03812464D04B9442DEh"));
			test_asserts::verify_equal ("initiator derives the RFC secret", scheme.mix (initiator_private, responder_public), expected);
			test_asserts::verify_equal ("responder derives the RFC secret", scheme.mix (responder_private, initiator_public), expected);
		});

		it ("preserves leading zero bytes in elliptic-curve shared secrets", []{
			auto const scheme = EllipticCurveScheme();
			auto const domain = CryptoPP::ECDH<CryptoPP::ECP>::Domain (CryptoPP::ASN1::secp256r1());
			Blob generator (domain.PublicKeyLength(), 0);
			domain.GetGroupParameters().EncodeElement (true, domain.GetGenerator(), generator.data());
			auto const private_value = p256_integer_bytes (CryptoPP::Integer (379));
			// The x coordinate of 379 * G starts with a zero byte.
			auto const expected = p256_integer_bytes (CryptoPP::Integer ("005543894af3d00ed7d740abdbd75c96b06877b787db5f70eea78b90a8d7c00ah"));
			Blob reference (domain.AgreedValueLength(), 0);
			test_asserts::verify ("Crypto++ accepts the reference exchange", domain.Agree (reference.data(), private_value.data(), generator.data()));
			test_asserts::verify_equal ("Crypto++ confirms the fixed leading-zero fixture", reference, expected);
			test_asserts::verify_equal ("the result retains all 32 bytes", scheme.mix (private_value, generator), expected);
		});

		it ("keeps elliptic-curve key generation and agreement interoperable with Crypto++", []{
			auto device = std::random_device ("hw");
			auto rng = CryptoPP::AutoSeededRandomPool();
			auto const scheme = EllipticCurveScheme();
			auto domain = CryptoPP::ECDH<CryptoPP::ECP>::Domain (CryptoPP::ASN1::secp256r1());

			for (unsigned int i = 0; i < 8; ++i)
			{
				auto const exchange = scheme.generate_exchange (device);
				test_asserts::verify_equal ("private scalars retain their fixed width", exchange.private_value->size(), std::size_t (32));
				test_asserts::verify_equal ("public shares retain their uncompressed width", exchange.public_share.size(), std::size_t (65));
				test_asserts::verify_equal ("the advertised maximum remains 65 bytes", scheme.max_blob_size(), std::size_t (65));

				auto const scalar = CryptoPP::Integer (exchange.private_value->data(), exchange.private_value->size());
				test_asserts::verify ("generated private scalars lie in [1, n - 1]",
									  scalar.IsPositive() && scalar < domain.GetGroupParameters().GetSubgroupOrder());

				Blob reference_public (domain.PublicKeyLength(), 0);
				domain.GeneratePublicKey (rng, exchange.private_value->data(), reference_public.data());
				test_asserts::verify_equal ("Crypto++ produces the same public share", exchange.public_share, reference_public);

				auto reference_private = CryptoPP::SecByteBlock (domain.PrivateKeyLength());
				Blob peer_public (domain.PublicKeyLength(), 0);
				domain.GenerateKeyPair (rng, reference_private.data(), peer_public.data());
				Blob reference_secret (domain.AgreedValueLength(), 0);
				test_asserts::verify ("the Crypto++ peer accepts the OpenSSL public share",
									  domain.Agree (reference_secret.data(), reference_private.data(), exchange.public_share.data()));
				test_asserts::verify_equal ("both implementations derive the same fixed-width secret",
											scheme.mix (*exchange.private_value, peer_public), reference_secret);
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

			auto const domain = CryptoPP::ECDH<CryptoPP::ECP>::Domain (CryptoPP::ASN1::secp256r1());
			Blob generator (domain.PublicKeyLength(), 0);
			domain.GetGroupParameters().EncodeElement (true, domain.GetGenerator(), generator.data());
			auto const private_value = p256_integer_bytes (CryptoPP::Integer::One());
			Blob off_curve (65, 0);
			off_curve[0] = 0x04;
			auto corrupted = generator;
			corrupted.back() ^= 1;
			auto const field_prime = p256_integer_bytes (domain.GetGroupParameters().GetCurve().GetField().GetModulus());
			Blob const invalid_points[] {
				Blob { 0 }, Blob (65, 0), off_curve, corrupted,
				Blob { 0x04 } + field_prime + generator.substr (33),
				generator.substr (0, 33) + field_prime,
				Blob { 0x03 } + generator.substr (1, 32),
				generator + Blob { 0 },
			};

			for (auto const& point: invalid_points)
			{
				test_asserts::verify_throws<InvalidArgument> ("infinity, off-curve, noncanonical and wrongly sized points are rejected", [&]{
					[[maybe_unused]] auto const key = scheme.mix (private_value, point);
				});
			}

			for (auto const prefix: { 0x00, 0x02, 0x03, 0x05, 0x06, 0x07, 0xff })
			{
				auto point = generator;
				point[0] = static_cast<Blob::value_type> (prefix);
				test_asserts::verify_throws<InvalidArgument> ("only the uncompressed SEC1 encoding is accepted", [&]{
					[[maybe_unused]] auto const key = scheme.mix (private_value, point);
				});
			}
		});

		it ("rejects unsupported elliptic curves in all entry points", []{
			auto const scheme = EllipticCurveScheme (static_cast<EllipticCurveScheme::EllipticCurve> (0xff));
			auto device = std::random_device ("hw");
			test_asserts::verify_throws<InvalidArgument> ("generation rejects an unknown curve", [&]{
				[[maybe_unused]] auto const exchange = scheme.generate_exchange (device);
			});
			test_asserts::verify_throws<InvalidArgument> ("agreement rejects an unknown curve", [&]{
				[[maybe_unused]] auto const key = scheme.mix (Blob (32, 1), Blob (65, 1));
			});
			test_asserts::verify_throws<InvalidArgument> ("size queries reject an unknown curve", [&]{
				[[maybe_unused]] auto const size = scheme.max_blob_size();
			});
		});
	});
});

} // namespace
} // namespace neutrino::test

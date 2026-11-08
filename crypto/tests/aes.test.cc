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
#include <neutrino/crypto/aes.h>
#include <neutrino/string.h>

// Crypto++:
#include <cryptopp/modes.h>

// Standard:
#include <cstddef>
#include <limits>
#include <stdexcept>


namespace neutrino::test {
namespace {

TestRegistration tests ([]{
	describe ("Neutrino", "crypto", []{
		it ("aes_ctr_xor()", []{
			Blob data1 ({ 0x00, 0x01, 0x05, 0x10, 0x12, 0x15, 0x00, 0xf0, 0xfa, 0x2f, 0x3f, 0x55, 0x6f, 0x8a, 0xca, 0xd0 });
			Blob encrypted1 = aes_ctr_xor ({
				.data = data1,
				.key = data1,
				.nonce = Blob (8, 0x00),
			});

			test_asserts::verify ("computed encrypted result (1) is correct", to_hex_string (encrypted1) == "da80702714e2170902465d3db3405cda");

			Blob data2 ({ 0x00, 0x01, 0x05, 0x10, 0x12, 0x15, 0x00, 0xf0, 0xfa, 0x2f, 0x3f, 0x55, 0x6f, 0x8a, 0xca, 0xd0, 0xfa, 0xdf, 0xca, 0xf8, 0x55, 0x66, 0x99, 0xaa });
			Blob encrypted2 = aes_ctr_xor ({
				.data = data2,
				.key = data1,
				.nonce = Blob (8, 0x00),
			});

			test_asserts::verify ("computed encrypted result (2) is correct", to_hex_string (encrypted2) == "da80702714e2170902465d3db3405cda2f1a8f4bd065952b");
		});

		it ("matches reference AES-CTR for all nonce and key sizes", []{
			for (auto const nonce_size: { 8u, 12u, 16u })
			{
				Blob const nonce (nonce_size, 0xad);
				Blob const iv = nonce + Blob (16 - nonce_size, 0);

				for (auto const key_size: { 16u, 24u, 32u })
				{
					Blob const key (key_size, 0x5a);

					for (auto const length: { 0u, 1u, 15u, 16u, 17u, 31u, 32u, 4097u })
					{
						if (nonce_size == 16 && length > 16)
							continue;

						Blob input (length, 0);

						for (size_t i = 0; i < input.size(); ++i)
							input[i] = static_cast<Blob::value_type> (17 * i);

						Blob expected (length, 0);
						CryptoPP::CTR_Mode<CryptoPP::AES>::Encryption reference;
						reference.SetKeyWithIV (key.data(), key.size(), iv.data(), iv.size());
						reference.ProcessData (expected.data(), input.data(), input.size());
						auto const actual = aes_ctr_xor ({ .data = input, .key = key, .nonce = nonce });
						test_asserts::verify_equal ("blockwise CTR agrees with Crypto++", actual, expected);
						test_asserts::verify_equal ("CTR decrypts partial final blocks", aes_ctr_xor ({ .data = actual, .key = key, .nonce = nonce }), input);
					}
				}
			}
		});

		it ("rejects AES-CTR counter exhaustion before encryption", []{
			// Exercise the actual length validator without allocating a 64 GiB buffer.
			for (auto const nonce_size: { 8u, 12u, 16u })
				for (auto const data_size: { 0u, 1u, 15u, 16u })
					detail::validate_aes_ctr_lengths (nonce_size, data_size);

			if constexpr (std::numeric_limits<size_t>::digits > 36)
			{
				auto const max_data_size = static_cast<size_t> (uint64_t { 1 } << 36);
				detail::validate_aes_ctr_lengths (12, max_data_size - 1);
				detail::validate_aes_ctr_lengths (12, max_data_size);
				test_asserts::verify_throws<std::length_error> ("a 96-bit nonce rejects even one byte beyond counter capacity", [&]{
					detail::validate_aes_ctr_lengths (12, max_data_size + 1);
				});
				test_asserts::verify_throws<std::length_error> ("length validation cannot overflow near SIZE_MAX", []{
					detail::validate_aes_ctr_lengths (12, std::numeric_limits<size_t>::max());
				});
			}

			detail::validate_aes_ctr_lengths (8, std::numeric_limits<size_t>::max());
			test_asserts::verify_throws<std::length_error> ("a 128-bit nonce rejects huge data sizes", []{
				detail::validate_aes_ctr_lengths (16, std::numeric_limits<size_t>::max());
			});

			for (auto const data_size: { 17u, 32u })
			{
				Blob const data (data_size, 0);
				Blob const key (16, 0x5a);
				Blob const nonce (16, 0xad);
				test_asserts::verify_throws<std::length_error> ("a 128-bit nonce cannot encrypt a second block", [&]{
					[[maybe_unused]] auto const output = aes_ctr_xor ({ .data = data, .key = key, .nonce = nonce });
				});
			}
		});

		it ("validates AES-CTR parameters even for empty input", []{
			test_asserts::verify_throws ("empty data does not accept an empty key and nonce", []{
				[[maybe_unused]] auto const output = aes_ctr_xor ({ .data = {}, .key = {}, .nonce = {} });
			});

			Blob const valid_key (16, 0x5a);

			for (auto const length: { 0u, 1u })
			{
				Blob const data (length, 0x5a);

				for (auto const nonce_length: { 8u, 12u, 16u })
				{
					Blob const nonce (nonce_length, 0xad);

					for (auto const key_length: { 0u, 1u, 15u, 17u, 23u, 25u, 31u, 33u })
					{
						Blob const key (key_length, 0x5a);
						test_asserts::verify_throws<CryptoPP::InvalidKeyLength> ("invalid key lengths are rejected regardless of data length", [&]{
							[[maybe_unused]] auto const output = aes_ctr_xor ({ .data = data, .key = key, .nonce = nonce });
						});
					}
				}

				for (auto const nonce_length: { 0u, 1u, 7u, 9u, 11u, 13u, 15u, 17u })
				{
					Blob const nonce (nonce_length, 0xad);
					test_asserts::verify_throws<std::runtime_error> ("unsupported nonce lengths are rejected regardless of data length", [&]{
						[[maybe_unused]] auto const output = aes_ctr_xor ({ .data = data, .key = valid_key, .nonce = nonce });
					});
				}
			}
		});
	});
});

} // namespace
} // namespace neutrino::test

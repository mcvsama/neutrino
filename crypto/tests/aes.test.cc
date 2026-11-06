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

		it ("preserves AES-CTR output across block boundaries and key sizes", []{
			Blob const nonce (8, 0xad);
			Blob const iv = nonce + Blob (8, 0);

			for (auto const key_size: { 16u, 24u, 32u })
			{
				Blob const key (key_size, 0x5a);

				for (auto const length: { 0u, 1u, 15u, 16u, 17u, 31u, 32u, 4097u })
				{
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
		});
	});
});

} // namespace
} // namespace neutrino::test

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
#include <neutrino/crypto/hkdf.h>
#include <neutrino/string.h>

// Crypto++:
#include <cryptopp/hkdf.h>

// Standard:
#include <cstddef>
#include <limits>


namespace neutrino::test {
namespace {

template<Hash::Algorithm Algorithm, class ReferenceHash>
	void
	verify_hkdf_length_limits()
	{
		Blob const salt = to_blob ("salt");
		Blob const key_material = to_blob ("key");
		Blob const info = to_blob ("info");
		auto const reference = CryptoPP::HKDF<ReferenceHash>();
		auto const max_length = reference.MaxDerivedKeyLength();
		auto const hash_length = std::size_t (ReferenceHash::DIGESTSIZE);
		std::size_t const valid_lengths[] { 0, 1, hash_length + 1, max_length - 1, max_length };

		for (auto const length: valid_lengths)
		{
			auto const output = compute_hkdf<Algorithm> ({
				.salt = salt,
				.key_material = key_material,
				.info = info,
				.result_length = length,
			});
			test_asserts::verify_equal ("valid output lengths, including zero and 255 blocks, are preserved", output.size(), length);

			// Crypto++ requires nonempty output; zero-length output is checked above.
			if (length > 0)
			{
				Blob expected (length, 0);
				reference.DeriveKey (expected.data(), expected.size(), key_material.data(), key_material.size(),
									 salt.data(), salt.size(), info.data(), info.size());
				test_asserts::verify_equal ("HKDF agrees with Crypto++ through the last allowed counter value", output, expected);
			}
		}

		for (auto const length: { max_length + 1, max_length + hash_length, std::numeric_limits<std::size_t>::max() })
		{
			test_asserts::verify_throws<InvalidArgument> ("oversized output is rejected before allocation or counter wraparound", [&]{
				[[maybe_unused]] auto const output = compute_hkdf<Algorithm> ({
					.salt = salt,
					.key_material = key_material,
					.info = info,
					.result_length = length,
				});
			});
		}
	}


TestRegistration tests ([]{
	describe ("Neutrino", "crypto", []{
		it ("compute_hkdf()", []{
			auto const output_key = compute_hkdf<Hash::SHA2_256> ({
				.salt = to_blob ("salt"),
				.key_material = to_blob ("key"),
				.info = to_blob ("info"),
				.result_length = 64,
			});

			test_asserts::verify ("computed output key is correct", to_hex_string (output_key) == "9ca0d662557439e3b83365f2da4626d35da195c6d9d1779f09838cf9e408966ece99106e2585ace6f083fa1acd5192f430fd55bc741184dbb822fc04c3ebfd3d");
		});

		it ("enforces HKDF output length limits for every supported hash", []{
			verify_hkdf_length_limits<Hash::SHA2_256, CryptoPP::SHA256>();
			verify_hkdf_length_limits<Hash::SHA3_224, CryptoPP::SHA3_224>();
			verify_hkdf_length_limits<Hash::SHA3_256, CryptoPP::SHA3_256>();
			verify_hkdf_length_limits<Hash::SHA3_384, CryptoPP::SHA3_384>();
			verify_hkdf_length_limits<Hash::SHA3_512, CryptoPP::SHA3_512>();
		});
	});
});

} // namespace
} // namespace neutrino::test

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
#include <neutrino/crypto/hmac.h>
#include <neutrino/string.h>

// Standard:
#include <cstddef>


namespace neutrino::test {
namespace {

template<Hash::Algorithm Algorithm>
	void
	verify_hmac_keys (std::string const& empty_expected, std::string const& long_expected)
	{
		// Reference values generated with Python's hmac/hashlib (OpenSSL backend).
		auto const empty = compute_hmac<Algorithm> ({ .data = {}, .key = {} });
		test_asserts::verify_equal ("empty HMAC keys and messages remain supported", to_hex_string (empty), empty_expected);

		auto const long_key = compute_hmac<Algorithm> ({ .data = Blob (50, 0xdd), .key = Blob (200, 0xaa) });
		test_asserts::verify_equal ("keys longer than the hash block are reduced correctly", to_hex_string (long_key), long_expected);
	}


TestRegistration tests ([]{
	describe ("Neutrino", "crypto", []{
		it ("compute_hmac()", []{
			auto const hmac = compute_hmac<Hash::SHA2_256> ({
				.data = to_blob ("data"),
				.key = to_blob ("key"),
			});

			test_asserts::verify ("computed HMAC is correct", to_hex_string (hmac) == "5031fe3d989c6d1537a013fa6e739da23463fdaec3b70137d828e36ace221bd0");
		});

		it ("preserves empty and long HMAC keys for every supported hash", []{
			verify_hmac_keys<Hash::SHA2_256> (
				"b613679a0814d9ec772f95d778c35fc5ff1697c493715653c6c712144292c5ad",
				"3794576826efdb1762339a40a6f3f0533da9f21c47175491495b3151a6be78be"
			);
			verify_hmac_keys<Hash::SHA3_224> (
				"1b9044e0d5bb4ef944bc00f1b26c483ac3e222f4640935d089a49083",
				"4d73592f4d6a8eb1ae4ba2c5ee83911dd221a522884249cafd27cd75"
			);
			verify_hmac_keys<Hash::SHA3_256> (
				"e841c164e5b4f10c9f3985587962af72fd607a951196fc92fb3a5251941784ea",
				"ac2e53b0bc120bfc894a7ca7071cfec938a4d39595424f193e05ea6ab2baf882"
			);
			verify_hmac_keys<Hash::SHA3_384> (
				"adca89f07bbfbeaf58880c1572379ea2416568fd3b66542bd42599c57c4567e6ae086299ea216c6f3e7aef90b6191d24",
				"69755fdf77b8d6f4c1f0d01dd5ec9c431a1a8b7ff955a3119e210c288794693362bd9837118dd8751c1a70ecb0c22418"
			);
			verify_hmac_keys<Hash::SHA3_512> (
				"cbcf45540782d4bc7387fbbf7d30b3681d6d66cc435cafd82546b0fce96b367ea79662918436fba442e81a01d0f9592dfcd30f7a7a8f1475693d30be4150ca84",
				"58397331b3b4da3aaa178bebe5c611234b7d49994bbfb8048c3ef3119cbbca83575d647f575890439276a721a13477061237c14d16806d649e3f10883d03079f"
			);
		});

	});
});

} // namespace
} // namespace neutrino::test

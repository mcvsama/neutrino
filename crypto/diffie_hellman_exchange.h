/* vim:ts=4
 *
 * Copyleft 2012…2016  Michał Gawron
 * Marduk Unix Labs, http://mulabs.org/
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Visit http://www.gnu.org/licenses/gpl-3.0.html for more information on licensing.
 */

#ifndef NEUTRINO__CRYPTO__DIFFIE_HELLMAN_EXCHANGE_H__INCLUDED
#define NEUTRINO__CRYPTO__DIFFIE_HELLMAN_EXCHANGE_H__INCLUDED

// Standard:
#include <cstddef>

// Boost:
#include <boost/random.hpp>
#include <boost/random/random_device.hpp>
#include <boost/multiprecision/cpp_int.hpp>

// Neutrino:
#include <neutrino/blob.h>


namespace neutrino {

class DiffieHellmanExchange
{
  public:
	using Integer = boost::multiprecision::cpp_int;

	class Parameters
	{
	  public:
		size_t	bits;
		Integer	shared_base;
		Integer	shared_modulo;
	};

	// 2048-bit MODP Group ID=14 from <https://www.ietf.org/rfc/rfc3526.txt>:
	static inline Parameters const RFC3526_GROUP14 {
		.bits			= 2048,
		.shared_base	= 2,
		.shared_modulo	= Integer (
			"0x"
			"FFFFFFFFFFFFFFFFC90FDAA22168C234C4C6628B80DC1CD1"
			"29024E088A67CC74020BBEA63B139B22514A08798E3404DD"
			"EF9519B3CD3A431B302B0A6DF25F14374FE1356D6D51C245"
			"E485B576625E7EC6F44C42E9A637ED6B0BFF5CB6F406B7ED"
			"EE386BFB5A899FA5AE9F24117C4B1FE649286651ECE45B3D"
			"C2007CB8A163BF0598DA48361C55D39A69163FA8FD24CF5F"
			"83655D23DCA3AD961C62F356208552BB9ED529077096966D"
			"670C354E4ABC9804F1746C08CA18217C32905E462E36CE3B"
			"E39E772C180E86039B2783A2EC07A28FB5C55DF06F4C52C9"
			"DE2BCBF6955817183995497CEA956AE515D2261898FA0510"
			"15728E5A8AACAA68FFFFFFFFFFFFFFFF"
		),
	};

	// 3072-bit MODP Group ID=15 from <https://www.ietf.org/rfc/rfc3526.txt>:
	static inline Parameters const RFC3526_GROUP15 {
		.bits			= 3072,
		.shared_base	= 2,
		.shared_modulo	= Integer (
			"0x"
			"FFFFFFFFFFFFFFFFC90FDAA22168C234C4C6628B80DC1CD1"
			"29024E088A67CC74020BBEA63B139B22514A08798E3404DD"
			"EF9519B3CD3A431B302B0A6DF25F14374FE1356D6D51C245"
			"E485B576625E7EC6F44C42E9A637ED6B0BFF5CB6F406B7ED"
			"EE386BFB5A899FA5AE9F24117C4B1FE649286651ECE45B3D"
			"C2007CB8A163BF0598DA48361C55D39A69163FA8FD24CF5F"
			"83655D23DCA3AD961C62F356208552BB9ED529077096966D"
			"670C354E4ABC9804F1746C08CA18217C32905E462E36CE3B"
			"E39E772C180E86039B2783A2EC07A28FB5C55DF06F4C52C9"
			"DE2BCBF6955817183995497CEA956AE515D2261898FA0510"
			"15728E5A8AAAC42DAD33170D04507A33A85521ABDF1CBA64"
			"ECFB850458DBEF0A8AEA71575D060C7DB3970F85A6E1E4C7"
			"ABF5AE8CDB0933D71E8C94E04A25619DCEE3D2261AD2EE6B"
			"F12FFA06D98A0864D87602733EC86A64521F2B18177B200C"
			"BBE117577A615D6C770988C0BAD946E208E24FA074E5AB31"
			"43DB5BFCE0FD108E4B82D120A93AD2CAFFFFFFFFFFFFFFFF"
		),
	};

	// 4096-bit MODP Group ID=16 from <https://www.ietf.org/rfc/rfc3526.txt>:
	static inline Parameters const RFC3526_GROUP16 {
		.bits			= 4096,
		.shared_base	= 2,
		.shared_modulo	= Integer (
			"0x"
			"FFFFFFFFFFFFFFFFC90FDAA22168C234C4C6628B80DC1CD1"
			"29024E088A67CC74020BBEA63B139B22514A08798E3404DD"
			"EF9519B3CD3A431B302B0A6DF25F14374FE1356D6D51C245"
			"E485B576625E7EC6F44C42E9A637ED6B0BFF5CB6F406B7ED"
			"EE386BFB5A899FA5AE9F24117C4B1FE649286651ECE45B3D"
			"C2007CB8A163BF0598DA48361C55D39A69163FA8FD24CF5F"
			"83655D23DCA3AD961C62F356208552BB9ED529077096966D"
			"670C354E4ABC9804F1746C08CA18217C32905E462E36CE3B"
			"E39E772C180E86039B2783A2EC07A28FB5C55DF06F4C52C9"
			"DE2BCBF6955817183995497CEA956AE515D2261898FA0510"
			"15728E5A8AAAC42DAD33170D04507A33A85521ABDF1CBA64"
			"ECFB850458DBEF0A8AEA71575D060C7DB3970F85A6E1E4C7"
			"ABF5AE8CDB0933D71E8C94E04A25619DCEE3D2261AD2EE6B"
			"F12FFA06D98A0864D87602733EC86A64521F2B18177B200C"
			"BBE117577A615D6C770988C0BAD946E208E24FA074E5AB31"
			"43DB5BFCE0FD108E4B82D120A92108011A723C12A787E6D7"
			"88719A10BDBA5B2699C327186AF4E23C1A946834B6150BDA"
			"2583E9CA2AD44CE8DBBBC2DB04DE8EF92E8EFC141FBECAA6"
			"287C59474E6BC05D99B2964FA090C3A2233BA186515BE7ED"
			"1F612970CEE2D7AFB81BDD762170481CD0069127D5B05AA9"
			"93B4EA988D8FDDC186FFB7DC90A6C08F4DF435C934063199"
			"FFFFFFFFFFFFFFFF"
		),
	};

  public:
	// Ctor
	explicit
	DiffieHellmanExchange (boost::random::random_device&);

	// Ctor
	explicit
	DiffieHellmanExchange (boost::random::random_device&, Parameters const&);

	// Dtor
	~DiffieHellmanExchange();

	/**
	 * Return value for exchange with the other party.
	 * Call this each time for new DHE exchange. It generates new secret value on each call.
	 */
	Blob
	generate_exchange_blob();

	/**
	 * Finalize exchange and return secret key to use.
	 * It's best to hash the result and use the hashed value as a key.
	 */
	Blob
	calculate_key_with_weak_bits (Blob const& other_exchange_blob) const;

	/**
	 * Return number of bytes required to store the exchange integer and the calculated key.
	 * This depends on the shared modulo value.
	 */
	size_t
	max_blob_size() const
		{ return 8 * msb (_parameters.shared_modulo); }

  public:
	static Blob
	to_blob (Integer const&, size_t reserve_bytes);

	static Integer
	to_integer (BlobView const blob);

  private:
	static Integer
	mix (Integer const& shared_base,
		 Integer const& shared_modulo,
		 Integer const& secret);

  private:
	boost::random::random_device&	_random_device;
	Parameters						_parameters;
	Integer							_secret_value;
};

} // namespace neutrino

#endif


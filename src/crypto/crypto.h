// Copyright (c) 2019, Ryo Currency Project
// Portions copyright (c) 2014-2018, The Monero Project
//
// Portions of this file are available under BSD-3 license. Please see ORIGINAL-LICENSE for details
// All rights reserved.
//
// Authors and copyright holders give permission for following:
//
// 1. Redistribution and use in source and binary forms WITHOUT modification.
//
// 2. Modification of the source form for your own personal use.
//
// As long as the following conditions are met:
//
// 3. You must not distribute modified copies of the work to third parties. This includes
//    posting the work online, or hosting copies of the modified work for download.
//
// 4. Any derivative version of this work is also covered by this license, including point 8.
//
// 5. Neither the name of the copyright holders nor the names of the authors may be
//    used to endorse or promote products derived from this software without specific
//    prior written permission.
//
// 6. You agree that this licence is governed by and shall be construed in accordance
//    with the laws of England and Wales.
//
// 7. You agree to submit all disputes arising out of or in connection with this licence
//    to the exclusive jurisdiction of the Courts of England and Wales.
//
// Authors and copyright holders agree that:
//
// 8. This licence expires and the work covered by it is released into the
//    public domain on 1st of February 2020
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
// THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
// THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Parts of this file are originally copyright (c) 2012-2013 The Cryptonote developers

#pragma once

#include <boost/optional.hpp>
#include <boost/thread/lock_guard.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/utility/value_init.hpp>
#include <cstddef>
#include <iostream>
#include <type_traits>
#include <vector>

#include "common/util.h"
#include "fmt/format.h"
#include "generic-ops.h"
#include "hash.h"
#include "hex.h"
#include "memwipe.h"
#include "random.hpp"
#include "span.h"

namespace crypto
{

#pragma pack(push, 1)
struct ec_point
{
	char data[32];
};

struct ec_scalar
{
	char data[32];
};

struct public_key : ec_point
{
	friend class crypto_ops;
};

struct scalar_16
{
	uint8_t data[16];
};

using secret_key_16 = tools::scrubbed<scalar_16>;
using secret_key = tools::scrubbed<ec_scalar>;

struct public_keyV
{
	std::vector<public_key> keys;
	int rows;
};

struct secret_keyV
{
	std::vector<secret_key> keys;
	int rows;
};

struct public_keyM
{
	int cols;
	int rows;
	std::vector<secret_keyV> column_vectors;
};

struct key_derivation : ec_point
{
	friend class crypto_ops;
};

struct key_image : ec_point
{
	friend class crypto_ops;
};

struct signature
{
	ec_scalar c, r;
	friend class crypto_ops;
};

// New payment id system
struct uniform_payment_id
{
	uint64_t zero = (-1); // Zero field needs to equal zero for the structure to be valid
	hash payment_id = null_hash;
};
#pragma pack(pop)

void hash_to_scalar(const void *data, size_t length, ec_scalar &res);
void random_scalar(unsigned char *v32);

static_assert(sizeof(ec_point) == 32 && sizeof(ec_scalar) == 32 &&
		sizeof(public_key) == 32 && sizeof(secret_key) == 32 &&
		sizeof(key_derivation) == 32 && sizeof(key_image) == 32 &&
		sizeof(signature) == 64 && sizeof(uniform_payment_id) == 40,
	"Invalid structure size");

static constexpr uint32_t KEY_VARIANT_KURZ = 0;
static constexpr uint32_t KEY_VARIANT_VIEWKEY = 1;
static constexpr uint32_t KEY_VARIANT_SPENDKEY = 2;

class crypto_ops
{
	crypto_ops();
	crypto_ops(const crypto_ops &);
	void operator=(const crypto_ops &);
	~crypto_ops();

	static secret_key generate_legacy_keys(public_key &pub, secret_key &sec, const secret_key &recovery_key = secret_key(), bool recover = false);
	friend secret_key generate_legacy_keys(public_key &pub, secret_key &sec, const secret_key &recovery_key, bool recover);

	static bool check_key(const public_key &);
	friend bool check_key(const public_key &);
	static bool secret_key_to_public_key(const secret_key &, public_key &);
	friend bool secret_key_to_public_key(const secret_key &, public_key &);
	static bool generate_key_derivation(const public_key &, const secret_key &, key_derivation &);
	friend bool generate_key_derivation(const public_key &, const secret_key &, key_derivation &);
	static void derivation_to_scalar(const key_derivation &derivation, size_t output_index, ec_scalar &res);
	friend void derivation_to_scalar(const key_derivation &derivation, size_t output_index, ec_scalar &res);
	static bool derive_public_key(const key_derivation &, std::size_t, const public_key &, public_key &);
	friend bool derive_public_key(const key_derivation &, std::size_t, const public_key &, public_key &);
	static void derive_secret_key(const key_derivation &, std::size_t, const secret_key &, secret_key &);
	friend void derive_secret_key(const key_derivation &, std::size_t, const secret_key &, secret_key &);
	static bool derive_subaddress_public_key(const public_key &, const key_derivation &, std::size_t, public_key &);
	friend bool derive_subaddress_public_key(const public_key &, const key_derivation &, std::size_t, public_key &);
#ifdef HAVE_EC_64
	static bool generate_key_derivation_64(const public_key &, const secret_key &, key_derivation &);
	friend bool generate_key_derivation_64(const public_key &, const secret_key &, key_derivation &);
	static bool derive_subaddress_public_key_64(const public_key &, const key_derivation &, std::size_t, public_key &);
	friend bool derive_subaddress_public_key_64(const public_key &, const key_derivation &, std::size_t, public_key &);
#endif
	static void generate_signature(const hash &, const public_key &, const secret_key &, signature &);
	friend void generate_signature(const hash &, const public_key &, const secret_key &, signature &);
	static bool check_signature(const hash &, const public_key &, const signature &);
	friend bool check_signature(const hash &, const public_key &, const signature &);
	static void generate_tx_proof(const hash &, const public_key &, const public_key &, const boost::optional<public_key> &, const public_key &, const secret_key &, signature &);
	friend void generate_tx_proof(const hash &, const public_key &, const public_key &, const boost::optional<public_key> &, const public_key &, const secret_key &, signature &);
	static bool check_tx_proof(const hash &, const public_key &, const public_key &, const boost::optional<public_key> &, const public_key &, const signature &);
	friend bool check_tx_proof(const hash &, const public_key &, const public_key &, const boost::optional<public_key> &, const public_key &, const signature &);
	static void generate_key_image(const public_key &, const secret_key &, key_image &);
	friend void generate_key_image(const public_key &, const secret_key &, key_image &);
	static void generate_ring_signature(const hash &, const key_image &,
		const public_key *const *, std::size_t, const secret_key &, std::size_t, signature *);
	friend void generate_ring_signature(const hash &, const key_image &,
		const public_key *const *, std::size_t, const secret_key &, std::size_t, signature *);
	static bool check_ring_signature(const hash &, const key_image &,
		const public_key *const *, std::size_t, const signature *);
	friend bool check_ring_signature(const hash &, const key_image &,
		const public_key *const *, std::size_t, const signature *);
};

inline uniform_payment_id make_paymenet_id(const hash &long_id)
{
	uniform_payment_id id;
	id.zero = 0;
	id.payment_id = long_id;
	return id;
}

inline uniform_payment_id make_paymenet_id(const hash8 &short_id)
{
	uniform_payment_id id;
	id.zero = 0;
	memcpy(&id.payment_id.data, &short_id.data, sizeof(hash8));
	return id;
}

/* Generate N random bytes
   */
inline void rand(size_t N, uint8_t *bytes)
{
	prng::inst().generate_random(bytes, N);
}

/* Generate a value filled with random bytes.
   */
template <typename T>
typename std::enable_if<std::is_pod<T>::value, T>::type rand()
{
	typename std::remove_cv<T>::type res;
	prng::inst().generate_random(&res, sizeof(T));
	return res;
}

void generate_wallet_keys(public_key &pub, secret_key &sec, const secret_key_16 &wallet_secret, uint32_t key_variant);
void generate_wallet_secret(secret_key_16 &wallet_secret);

/* Generate a new key pair
   */
inline secret_key generate_legacy_keys(public_key &pub, secret_key &sec, const secret_key &recovery_key = secret_key(), bool recover = false)
{
	return crypto_ops::generate_legacy_keys(pub, sec, recovery_key, recover);
}

/* Check a public key. Returns true if it is valid, false otherwise.
   */
inline bool check_key(const public_key &key)
{
	return crypto_ops::check_key(key);
}

/* Checks a private key and computes the corresponding public key.
   */
inline bool secret_key_to_public_key(const secret_key &sec, public_key &pub)
{
	return crypto_ops::secret_key_to_public_key(sec, pub);
}

/* To generate an ephemeral key used to send money to:
   * * The sender generates a new key pair, which becomes the transaction key. The public transaction key is included in "extra" field.
   * * Both the sender and the receiver generate key derivation from the transaction key, the receivers' "view" key and the output index.
   * * The sender uses key derivation and the receivers' "spend" key to derive an ephemeral public key.
   * * The receiver can either derive the public key (to check that the transaction is addressed to him) or the private key (to spend the money).
   */
inline bool generate_key_derivation(const public_key &key1, const secret_key &key2, key_derivation &derivation)
{
	return crypto_ops::generate_key_derivation(key1, key2, derivation);
}
inline bool derive_public_key(const key_derivation &derivation, std::size_t output_index,
	const public_key &base, public_key &derived_key)
{
	return crypto_ops::derive_public_key(derivation, output_index, base, derived_key);
}
inline void derivation_to_scalar(const key_derivation &derivation, size_t output_index, ec_scalar &res)
{
	return crypto_ops::derivation_to_scalar(derivation, output_index, res);
}
inline void derive_secret_key(const key_derivation &derivation, std::size_t output_index,
	const secret_key &base, secret_key &derived_key)
{
	crypto_ops::derive_secret_key(derivation, output_index, base, derived_key);
}
inline bool derive_subaddress_public_key(const public_key &out_key, const key_derivation &derivation, std::size_t output_index, public_key &result)
{
	return crypto_ops::derive_subaddress_public_key(out_key, derivation, output_index, result);
}
#ifdef HAVE_EC_64
inline bool generate_key_derivation_64(const public_key &key1, const secret_key &key2, key_derivation &derivation)
{
	return crypto_ops::generate_key_derivation_64(key1, key2, derivation);
}

inline bool derive_subaddress_public_key_64(const public_key &out_key, const key_derivation &derivation, std::size_t output_index, public_key &result)
{
	return crypto_ops::derive_subaddress_public_key_64(out_key, derivation, output_index, result);
}
#endif

/* Generation and checking of a standard signature.
   */
inline void generate_signature(const hash &prefix_hash, const public_key &pub, const secret_key &sec, signature &sig)
{
	crypto_ops::generate_signature(prefix_hash, pub, sec, sig);
}
inline bool check_signature(const hash &prefix_hash, const public_key &pub, const signature &sig)
{
	return crypto_ops::check_signature(prefix_hash, pub, sig);
}

/* Generation and checking of a tx proof; given a tx pubkey R, the recipient's view pubkey A, and the key
   * derivation D, the signature proves the knowledge of the tx secret key r such that R=r*G and D=r*A
   * When the recipient's address is a subaddress, the tx pubkey R is defined as R=r*B where B is the recipient's spend pubkey
   */
inline void generate_tx_proof(const hash &prefix_hash, const public_key &R, const public_key &A, const boost::optional<public_key> &B, const public_key &D, const secret_key &r, signature &sig)
{
	crypto_ops::generate_tx_proof(prefix_hash, R, A, B, D, r, sig);
}
inline bool check_tx_proof(const hash &prefix_hash, const public_key &R, const public_key &A, const boost::optional<public_key> &B, const public_key &D, const signature &sig)
{
	return crypto_ops::check_tx_proof(prefix_hash, R, A, B, D, sig);
}

/* To send money to a key:
   * * The sender generates an ephemeral key and includes it in transaction output.
   * * To spend the money, the receiver generates a key image from it.
   * * Then he selects a bunch of outputs, including the one he spends, and uses them to generate a ring signature.
   * To check the signature, it is necessary to collect all the keys that were used to generate it. To detect double spends, it is necessary to check that each key image is used at most once.
   */
inline void generate_key_image(const public_key &pub, const secret_key &sec, key_image &image)
{
	crypto_ops::generate_key_image(pub, sec, image);
}
inline void generate_ring_signature(const hash &prefix_hash, const key_image &image,
	const public_key *const *pubs, std::size_t pubs_count,
	const secret_key &sec, std::size_t sec_index,
	signature *sig)
{
	crypto_ops::generate_ring_signature(prefix_hash, image, pubs, pubs_count, sec, sec_index, sig);
}
inline bool check_ring_signature(const hash &prefix_hash, const key_image &image,
	const public_key *const *pubs, std::size_t pubs_count,
	const signature *sig)
{
	return crypto_ops::check_ring_signature(prefix_hash, image, pubs, pubs_count, sig);
}

/* Variants with vector<const public_key *> parameters.
   */
inline void generate_ring_signature(const hash &prefix_hash, const key_image &image,
	const std::vector<const public_key *> &pubs,
	const secret_key &sec, std::size_t sec_index,
	signature *sig)
{
	generate_ring_signature(prefix_hash, image, pubs.data(), pubs.size(), sec, sec_index, sig);
}
inline bool check_ring_signature(const hash &prefix_hash, const key_image &image,
	const std::vector<const public_key *> &pubs,
	const signature *sig)
{
	return check_ring_signature(prefix_hash, image, pubs.data(), pubs.size(), sig);
}

inline std::ostream &operator<<(std::ostream &o, const crypto::public_key &v)
{
	epee::to_hex::formatted(o, epee::as_byte_span(v));
	return o;
}
inline std::ostream &operator<<(std::ostream &o, const crypto::secret_key &v)
{
	epee::to_hex::formatted(o, epee::as_byte_span(v));
	return o;
}
inline std::ostream &operator<<(std::ostream &o, const crypto::key_derivation &v)
{
	epee::to_hex::formatted(o, epee::as_byte_span(v));
	return o;
}
inline std::ostream &operator<<(std::ostream &o, const crypto::key_image &v)
{
	epee::to_hex::formatted(o, epee::as_byte_span(v));
	return o;
}
inline std::ostream &operator<<(std::ostream &o, const crypto::signature &v)
{
	epee::to_hex::formatted(o, epee::as_byte_span(v));
	return o;
}

const static crypto::public_key null_pkey = boost::value_initialized<crypto::public_key>();
const static crypto::secret_key null_skey = boost::value_initialized<crypto::secret_key>();
} // namespace crypto

namespace fmt
{
template <>
struct formatter<crypto::public_key> : formatter<string_view>
{
	template <typename FormatContext>
	auto format(const crypto::public_key &pk, FormatContext &ctx) -> decltype(ctx.out())
	{
		return formatter<string_view>::format(epee::string_tools::pod_to_hex(pk), ctx);
	}
};
} // namespace fmt

CRYPTO_MAKE_HASHABLE(public_key)
CRYPTO_MAKE_HASHABLE(secret_key)
CRYPTO_MAKE_HASHABLE(secret_key_16)
CRYPTO_MAKE_HASHABLE(key_image)
CRYPTO_MAKE_COMPARABLE(signature)

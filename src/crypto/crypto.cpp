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

#include <boost/shared_ptr.hpp>
#include <boost/thread/lock_guard.hpp>
#include <boost/thread/mutex.hpp>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <unistd.h>

#include "common/varint.h"
#include "crypto.h"
#include "hash.h"
#include "random.hpp"
#include "warnings.h"

#ifdef HAVE_EC_64
#include "ecops64/ecops64.h"
#endif

namespace
{
static void local_abort(const char *msg)
{
	fprintf(stderr, "%s\n", msg);
#ifdef NDEBUG
	_exit(1);
#else
	abort();
#endif
}
} // namespace

namespace crypto
{

using std::abort;
using std::int32_t;
using std::int64_t;
using std::size_t;
using std::uint32_t;
using std::uint64_t;

extern "C"
{
#include "crypto-ops.h"
}

static inline unsigned char *operator&(ec_point &point)
{
	return &reinterpret_cast<unsigned char &>(point);
}

static inline const unsigned char *operator&(const ec_point &point)
{
	return &reinterpret_cast<const unsigned char &>(point);
}

static inline unsigned char *operator&(ec_scalar &scalar)
{
	return &reinterpret_cast<unsigned char &>(scalar);
}

static inline const unsigned char *operator&(const ec_scalar &scalar)
{
	return &reinterpret_cast<const unsigned char &>(scalar);
}

static inline bool scalar_ok(const unsigned char *k)
{
	// l = 2^252 + 27742317777372353535851937790883648493
	// l15 = 15*l
	//unsigned char l15[32] = {
	//	0xe3, 0x6a, 0x67, 0x72, 0x8b, 0xce, 0x13, 0x29, 0x8f, 0x30, 0x82, 0x8c, 0x0b, 0xa4, 0x10, 0x39,
	//	0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0
	//};
	// Scalar above in little-endian words
	constexpr uint64_t lw3 = 0xf000000000000000ull;
	constexpr uint64_t lw2 = 0x0000000000000001ull;
	constexpr uint64_t lw1 = 0x3910a40b8c82308full;
	constexpr uint64_t lw0 = 0x2913ce8b72676ae3ull;

	const uint64_t *k64 = reinterpret_cast<const uint64_t *>(k);
	if(k64[0] == 0 && k64[1] == 0 && k64[2] == 0 && k64[3] == 0)
		return false;

	if(k64[3] < lw3)
		return true;
	if(k64[3] > lw3)
		return false;
	if(k64[2] < lw2)
		return true;
	if(k64[2] > lw2)
		return false;
	if(k64[1] < lw1)
		return true;
	if(k64[1] > lw1)
		return false;
	if(k64[0] < lw0)
		return true;
	return false;
}

void random_scalar(unsigned char *v32)
{
	do
	{
		prng::inst().generate_random(v32, 32);
	} while(!scalar_ok(v32));
	sc_reduce32(v32);
}

/* generate a random 32-byte (256-bit) integer and copy it to res */
static inline void random_scalar(ec_scalar &res)
{
	random_scalar(reinterpret_cast<unsigned char *>(res.data));
}

static inline void random_scalar(scalar_16 &res)
{
	prng::inst().generate_random(res.data, sizeof(res.data));
}

void hash_to_scalar(const void *data, size_t length, ec_scalar &res)
{
	cn_fast_hash(data, length, reinterpret_cast<hash &>(res));
	sc_reduce32(&res);
}

/*
   * generate public and secret keys from a random 256-bit integer
   * TODO: allow specifying random value (for wallet recovery)
   *
   */
secret_key crypto_ops::generate_legacy_keys(public_key &pub, secret_key &sec, const secret_key &recovery_key, bool recover)
{
	ge_p3 point;

	secret_key rng;

	if(recover)
	{
		rng = recovery_key;
	}
	else
	{
		random_scalar(rng);
	}
	sec = rng;
	sc_reduce32(&unwrap(sec)); // reduce in case second round of keys (sendkeys)

	ge_scalarmult_base(&point, &unwrap(sec));
	ge_p3_tobytes(&pub, &point);

	return rng;
}

void generate_wallet_secret(secret_key_16 &wallet_secret)
{
	random_scalar(unwrap(wallet_secret));
}

void generate_wallet_keys(public_key &pub, secret_key &sec, const secret_key_16 &wallet_secret, uint32_t key_variant)
{
#pragma pack(push, 1)
	struct hash_secret
	{
		uint32_t variant;
		scalar_16 secret;
	};
#pragma pack(pop)

	tools::scrubbed<hash_secret> hs;
	unwrap(hs).secret = unwrap(wallet_secret);
	unwrap(hs).variant = key_variant;

	hash_to_scalar(&unwrap(hs), sizeof(hash_secret), unwrap(sec));

	ge_p3 point;
	ge_scalarmult_base(&point, &unwrap(sec));
	ge_p3_tobytes(&pub, &point);
}

bool crypto_ops::check_key(const public_key &key)
{
	ge_p3 point;
	return ge_frombytes_vartime(&point, &key) == 0;
}

bool crypto_ops::secret_key_to_public_key(const secret_key &sec, public_key &pub)
{
	ge_p3 point;
	if(sc_check(&unwrap(sec)) != 0)
	{
		return false;
	}
	ge_scalarmult_base(&point, &unwrap(sec));
	ge_p3_tobytes(&pub, &point);
	return true;
}

bool crypto_ops::generate_key_derivation(const public_key &key1, const secret_key &key2, key_derivation &derivation)
{
	ge_p3 point;
	ge_p2 point2;
	ge_p1p1 point3;
	assert(sc_check(&key2) == 0);
	if(ge_frombytes_vartime(&point, &key1) != 0)
	{
		return false;
	}
	ge_scalarmult(&point2, &unwrap(key2), &point);
	ge_mul8(&point3, &point2);
	ge_p1p1_to_p2(&point2, &point3);
	ge_tobytes(&derivation, &point2);
	return true;
}

void crypto_ops::derivation_to_scalar(const key_derivation &derivation, size_t output_index, ec_scalar &res)
{
	struct
	{
		key_derivation derivation;
		char output_index[(sizeof(size_t) * 8 + 6) / 7];
	} buf;
	char *end = buf.output_index;
	buf.derivation = derivation;
	tools::write_varint(end, output_index);
	assert(end <= buf.output_index + sizeof buf.output_index);
	hash_to_scalar(&buf, end - reinterpret_cast<char *>(&buf), res);
}

bool crypto_ops::derive_public_key(const key_derivation &derivation, size_t output_index,
	const public_key &base, public_key &derived_key)
{
	ec_scalar scalar;
	ge_p3 point1;
	ge_p3 point2;
	ge_cached point3;
	ge_p1p1 point4;
	ge_p2 point5;
	if(ge_frombytes_vartime(&point1, &base) != 0)
	{
		return false;
	}
	derivation_to_scalar(derivation, output_index, scalar);
	ge_scalarmult_base(&point2, &scalar);
	ge_p3_to_cached(&point3, &point2);
	ge_add(&point4, &point1, &point3);
	ge_p1p1_to_p2(&point5, &point4);
	ge_tobytes(&derived_key, &point5);
	return true;
}

void crypto_ops::derive_secret_key(const key_derivation &derivation, size_t output_index,
	const secret_key &base, secret_key &derived_key)
{
	ec_scalar scalar;
	assert(sc_check(&base) == 0);
	derivation_to_scalar(derivation, output_index, scalar);
	sc_add(&unwrap(derived_key), &unwrap(base), &scalar);
}

bool crypto_ops::derive_subaddress_public_key(const public_key &out_key, const key_derivation &derivation, std::size_t output_index, public_key &derived_key)
{
	ec_scalar scalar;
	ge_p3 point1;
	ge_p3 point2;
	ge_cached point3;
	ge_p1p1 point4;
	ge_p2 point5;
	if(ge_frombytes_vartime(&point1, &out_key) != 0)
	{
		return false;
	}
	derivation_to_scalar(derivation, output_index, scalar);
	ge_scalarmult_base(&point2, &scalar);
	ge_p3_to_cached(&point3, &point2);
	ge_sub(&point4, &point1, &point3);
	ge_p1p1_to_p2(&point5, &point4);
	ge_tobytes(&derived_key, &point5);
	return true;
}

#ifdef HAVE_EC_64

bool crypto_ops::generate_key_derivation_64(const public_key &key1, const secret_key &key2, key_derivation &derivation)
{
	ge64_p3 point;
	ge64_p2 point2;
	ge64_p1p1 point3;
	assert(sc_check(&key2) == 0);
	if(ge64_frombytes_vartime(&point, &key1) != 0)
	{
		return false;
	}
	ge64_scalarmult(&point2, &unwrap(key2), &point);
	ge64_mul8(&point3, &point2);
	ge64_p1p1_to_p2(&point2, &point3);
	ge64_tobytes(&derivation, &point2);
	return true;
}

bool crypto_ops::derive_subaddress_public_key_64(const public_key &out_key, const key_derivation &derivation, const std::size_t output_index, public_key &derived_key)
{
	ec_scalar scalar;
	ge64_p3 point1;
	ge64_p3 point3;
	ge64_p1p1 point4;
	ge64_p2 point5;
	if(ge64_frombytes_vartime(&point1, &out_key) != 0)
	{
		return false;
	}
	derivation_to_scalar(derivation, output_index, scalar);
	ge64_scalarmult_base(&point3, &scalar);
	ge64_sub(&point4, &point1, &point3);
	ge64_p1p1_to_p2(&point5, &point4);
	ge64_tobytes(&derived_key, &point5);
	return true;
}

#endif

struct s_comm
{
	hash h;
	ec_point key;
	ec_point comm;
};

struct s_comm_2
{
	hash msg;
	ec_point D;
	ec_point X;
	ec_point Y;
};

void crypto_ops::generate_signature(const hash &prefix_hash, const public_key &pub, const secret_key &sec, signature &sig)
{
	ge_p3 tmp3;
	ec_scalar k;
	s_comm buf;
#if !defined(NDEBUG)
	{
		ge_p3 t;
		public_key t2;
		assert(sc_check(&sec) == 0);
		ge_scalarmult_base(&t, &sec);
		ge_p3_tobytes(&t2, &t);
		assert(pub == t2);
	}
#endif
	buf.h = prefix_hash;
	buf.key = pub;
	random_scalar(k);
	ge_scalarmult_base(&tmp3, &k);
	ge_p3_tobytes(&buf.comm, &tmp3);
	hash_to_scalar(&buf, sizeof(s_comm), sig.c);
	sc_mulsub(&sig.r, &sig.c, &unwrap(sec), &k);
}

bool crypto_ops::check_signature(const hash &prefix_hash, const public_key &pub, const signature &sig)
{
	ge_p2 tmp2;
	ge_p3 tmp3;
	ec_scalar c;
	s_comm buf;
	assert(check_key(pub));
	buf.h = prefix_hash;
	buf.key = pub;
	if(ge_frombytes_vartime(&tmp3, &pub) != 0)
	{
		return false;
	}
	if(sc_check(&sig.c) != 0 || sc_check(&sig.r) != 0)
	{
		return false;
	}
	ge_double_scalarmult_base_vartime(&tmp2, &sig.c, &tmp3, &sig.r);
	ge_tobytes(&buf.comm, &tmp2);
	hash_to_scalar(&buf, sizeof(s_comm), c);
	sc_sub(&c, &c, &sig.c);
	return sc_isnonzero(&c) == 0;
}

void crypto_ops::generate_tx_proof(const hash &prefix_hash, const public_key &R, const public_key &A, const boost::optional<public_key> &B, const public_key &D, const secret_key &r, signature &sig)
{
	// sanity check
	ge_p3 R_p3;
	ge_p3 A_p3;
	ge_p3 B_p3;
	ge_p3 D_p3;
	if(ge_frombytes_vartime(&R_p3, &R) != 0)
		throw std::runtime_error("tx pubkey is invalid");
	if(ge_frombytes_vartime(&A_p3, &A) != 0)
		throw std::runtime_error("recipient view pubkey is invalid");
	if(B && ge_frombytes_vartime(&B_p3, &*B) != 0)
		throw std::runtime_error("recipient spend pubkey is invalid");
	if(ge_frombytes_vartime(&D_p3, &D) != 0)
		throw std::runtime_error("key derivation is invalid");
#if !defined(NDEBUG)
	{
		assert(sc_check(&r) == 0);
		// check R == r*G or R == r*B
		public_key dbg_R;
		if(B)
		{
			ge_p2 dbg_R_p2;
			ge_scalarmult(&dbg_R_p2, &r, &B_p3);
			ge_tobytes(&dbg_R, &dbg_R_p2);
		}
		else
		{
			ge_p3 dbg_R_p3;
			ge_scalarmult_base(&dbg_R_p3, &r);
			ge_p3_tobytes(&dbg_R, &dbg_R_p3);
		}
		assert(R == dbg_R);
		// check D == r*A
		ge_p2 dbg_D_p2;
		ge_scalarmult(&dbg_D_p2, &r, &A_p3);
		public_key dbg_D;
		ge_tobytes(&dbg_D, &dbg_D_p2);
		assert(D == dbg_D);
	}
#endif

	// pick random k
	ec_scalar k;
	random_scalar(k);

	s_comm_2 buf;
	buf.msg = prefix_hash;
	buf.D = D;

	if(B)
	{
		// compute X = k*B
		ge_p2 X_p2;
		ge_scalarmult(&X_p2, &k, &B_p3);
		ge_tobytes(&buf.X, &X_p2);
	}
	else
	{
		// compute X = k*G
		ge_p3 X_p3;
		ge_scalarmult_base(&X_p3, &k);
		ge_p3_tobytes(&buf.X, &X_p3);
	}

	// compute Y = k*A
	ge_p2 Y_p2;
	ge_scalarmult(&Y_p2, &k, &A_p3);
	ge_tobytes(&buf.Y, &Y_p2);

	// sig.c = Hs(Msg || D || X || Y)
	hash_to_scalar(&buf, sizeof(buf), sig.c);

	// sig.r = k - sig.c*r
	sc_mulsub(&sig.r, &sig.c, &unwrap(r), &k);
}

bool crypto_ops::check_tx_proof(const hash &prefix_hash, const public_key &R, const public_key &A, const boost::optional<public_key> &B, const public_key &D, const signature &sig)
{
	// sanity check
	ge_p3 R_p3;
	ge_p3 A_p3;
	ge_p3 B_p3;
	ge_p3 D_p3;
	if(ge_frombytes_vartime(&R_p3, &R) != 0)
		return false;
	if(ge_frombytes_vartime(&A_p3, &A) != 0)
		return false;
	if(B && ge_frombytes_vartime(&B_p3, &*B) != 0)
		return false;
	if(ge_frombytes_vartime(&D_p3, &D) != 0)
		return false;
	if(sc_check(&sig.c) != 0 || sc_check(&sig.r) != 0)
		return false;

	// compute sig.c*R
	ge_p3 cR_p3;
	{
		ge_p2 cR_p2;
		ge_scalarmult(&cR_p2, &sig.c, &R_p3);
		public_key cR;
		ge_tobytes(&cR, &cR_p2);
		if(ge_frombytes_vartime(&cR_p3, &cR) != 0)
			return false;
	}

	ge_p1p1 X_p1p1;
	if(B)
	{
		// compute X = sig.c*R + sig.r*B
		ge_p2 rB_p2;
		ge_scalarmult(&rB_p2, &sig.r, &B_p3);
		public_key rB;
		ge_tobytes(&rB, &rB_p2);
		ge_p3 rB_p3;
		if(ge_frombytes_vartime(&rB_p3, &rB) != 0)
			return false;
		ge_cached rB_cached;
		ge_p3_to_cached(&rB_cached, &rB_p3);
		ge_add(&X_p1p1, &cR_p3, &rB_cached);
	}
	else
	{
		// compute X = sig.c*R + sig.r*G
		ge_p3 rG_p3;
		ge_scalarmult_base(&rG_p3, &sig.r);
		ge_cached rG_cached;
		ge_p3_to_cached(&rG_cached, &rG_p3);
		ge_add(&X_p1p1, &cR_p3, &rG_cached);
	}
	ge_p2 X_p2;
	ge_p1p1_to_p2(&X_p2, &X_p1p1);

	// compute sig.c*D
	ge_p2 cD_p2;
	ge_scalarmult(&cD_p2, &sig.c, &D_p3);

	// compute sig.r*A
	ge_p2 rA_p2;
	ge_scalarmult(&rA_p2, &sig.r, &A_p3);

	// compute Y = sig.c*D + sig.r*A
	public_key cD;
	public_key rA;
	ge_tobytes(&cD, &cD_p2);
	ge_tobytes(&rA, &rA_p2);
	ge_p3 cD_p3;
	ge_p3 rA_p3;
	if(ge_frombytes_vartime(&cD_p3, &cD) != 0)
		return false;
	if(ge_frombytes_vartime(&rA_p3, &rA) != 0)
		return false;
	ge_cached rA_cached;
	ge_p3_to_cached(&rA_cached, &rA_p3);
	ge_p1p1 Y_p1p1;
	ge_add(&Y_p1p1, &cD_p3, &rA_cached);
	ge_p2 Y_p2;
	ge_p1p1_to_p2(&Y_p2, &Y_p1p1);

	// compute c2 = Hs(Msg || D || X || Y)
	s_comm_2 buf;
	buf.msg = prefix_hash;
	buf.D = D;
	ge_tobytes(&buf.X, &X_p2);
	ge_tobytes(&buf.Y, &Y_p2);
	ec_scalar c2;
	hash_to_scalar(&buf, sizeof(s_comm_2), c2);

	// test if c2 == sig.c
	sc_sub(&c2, &c2, &sig.c);
	return sc_isnonzero(&c2) == 0;
}

static void hash_to_ec(const public_key &key, ge_p3 &res)
{
	hash h;
	ge_p2 point;
	ge_p1p1 point2;
	cn_fast_hash(std::addressof(key), sizeof(public_key), h);
	ge_fromfe_frombytes_vartime(&point, reinterpret_cast<const unsigned char *>(&h));
	ge_mul8(&point2, &point);
	ge_p1p1_to_p3(&res, &point2);
}

void crypto_ops::generate_key_image(const public_key &pub, const secret_key &sec, key_image &image)
{
	ge_p3 point;
	ge_p2 point2;
	assert(sc_check(&sec) == 0);
	hash_to_ec(pub, point);
	ge_scalarmult(&point2, &unwrap(sec), &point);
	ge_tobytes(&image, &point2);
}

PUSH_WARNINGS
DISABLE_VS_WARNINGS(4200)
struct ec_point_pair
{
	ec_point a, b;
};
struct rs_comm
{
	hash h;
	struct ec_point_pair ab[];
};
POP_WARNINGS

static inline size_t rs_comm_size(size_t pubs_count)
{
	return sizeof(rs_comm) + pubs_count * sizeof(ec_point_pair);
}

void crypto_ops::generate_ring_signature(const hash &prefix_hash, const key_image &image,
	const public_key *const *pubs, size_t pubs_count,
	const secret_key &sec, size_t sec_index,
	signature *sig)
{
	size_t i;
	ge_p3 image_unp;
	ge_dsmp image_pre;
	ec_scalar sum, k, h;
	boost::shared_ptr<rs_comm> buf(reinterpret_cast<rs_comm *>(malloc(rs_comm_size(pubs_count))), free);
	if(!buf)
		local_abort("malloc failure");
	assert(sec_index < pubs_count);
#if !defined(NDEBUG)
	{
		ge_p3 t;
		public_key t2;
		key_image t3;
		assert(sc_check(&sec) == 0);
		ge_scalarmult_base(&t, &sec);
		ge_p3_tobytes(&t2, &t);
		assert(*pubs[sec_index] == t2);
		generate_key_image(*pubs[sec_index], sec, t3);
		assert(image == t3);
		for(i = 0; i < pubs_count; i++)
		{
			assert(check_key(*pubs[i]));
		}
	}
#endif
	if(ge_frombytes_vartime(&image_unp, &image) != 0)
	{
		local_abort("invalid key image");
	}
	ge_dsm_precomp(image_pre, &image_unp);
	sc_0(&sum);
	buf->h = prefix_hash;
	for(i = 0; i < pubs_count; i++)
	{
		ge_p2 tmp2;
		ge_p3 tmp3;
		if(i == sec_index)
		{
			random_scalar(k);
			ge_scalarmult_base(&tmp3, &k);
			ge_p3_tobytes(&buf->ab[i].a, &tmp3);
			hash_to_ec(*pubs[i], tmp3);
			ge_scalarmult(&tmp2, &k, &tmp3);
			ge_tobytes(&buf->ab[i].b, &tmp2);
		}
		else
		{
			random_scalar(sig[i].c);
			random_scalar(sig[i].r);
			if(ge_frombytes_vartime(&tmp3, &*pubs[i]) != 0)
			{
				local_abort("invalid pubkey");
			}
			ge_double_scalarmult_base_vartime(&tmp2, &sig[i].c, &tmp3, &sig[i].r);
			ge_tobytes(&buf->ab[i].a, &tmp2);
			hash_to_ec(*pubs[i], tmp3);
			ge_double_scalarmult_precomp_vartime(&tmp2, &sig[i].r, &tmp3, &sig[i].c, image_pre);
			ge_tobytes(&buf->ab[i].b, &tmp2);
			sc_add(&sum, &sum, &sig[i].c);
		}
	}
	hash_to_scalar(buf.get(), rs_comm_size(pubs_count), h);
	sc_sub(&sig[sec_index].c, &h, &sum);
	sc_mulsub(&sig[sec_index].r, &sig[sec_index].c, &unwrap(sec), &k);
}

bool crypto_ops::check_ring_signature(const hash &prefix_hash, const key_image &image,
	const public_key *const *pubs, size_t pubs_count,
	const signature *sig)
{
	size_t i;
	ge_p3 image_unp;
	ge_dsmp image_pre;
	ec_scalar sum, h;
	boost::shared_ptr<rs_comm> buf(reinterpret_cast<rs_comm *>(malloc(rs_comm_size(pubs_count))), free);
	if(!buf)
		return false;
#if !defined(NDEBUG)
	for(i = 0; i < pubs_count; i++)
	{
		assert(check_key(*pubs[i]));
	}
#endif
	if(ge_frombytes_vartime(&image_unp, &image) != 0)
	{
		return false;
	}
	ge_dsm_precomp(image_pre, &image_unp);
	sc_0(&sum);
	buf->h = prefix_hash;
	for(i = 0; i < pubs_count; i++)
	{
		ge_p2 tmp2;
		ge_p3 tmp3;
		if(sc_check(&sig[i].c) != 0 || sc_check(&sig[i].r) != 0)
		{
			return false;
		}
		if(ge_frombytes_vartime(&tmp3, &*pubs[i]) != 0)
		{
			return false;
		}
		ge_double_scalarmult_base_vartime(&tmp2, &sig[i].c, &tmp3, &sig[i].r);
		ge_tobytes(&buf->ab[i].a, &tmp2);
		hash_to_ec(*pubs[i], tmp3);
		ge_double_scalarmult_precomp_vartime(&tmp2, &sig[i].r, &tmp3, &sig[i].c, image_pre);
		ge_tobytes(&buf->ab[i].b, &tmp2);
		sc_add(&sum, &sum, &sig[i].c);
	}
	hash_to_scalar(buf.get(), rs_comm_size(pubs_count), h);
	sc_sub(&h, &h, &sum);
	return sc_isnonzero(&h) == 0;
}
} // namespace crypto

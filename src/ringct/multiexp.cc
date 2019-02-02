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
// Adapted from Python code by Sarang Noether

#include "common/perf_timer.h"
#include "misc_log_ex.h"
extern "C" {
#include "crypto/crypto-ops.h"
}
#include "multiexp.h"
#include "rctOps.h"

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "multiexp"

//#define MULTIEXP_PERF(x) x
#define MULTIEXP_PERF(x)

//   per points us for N/B points (B point bands)
//   raw   alt   128/192  4096/192  4096/4096
//   0     0     52.6     71        71.2
//   0     1     53.2     72.2      72.4
//   1     0     52.7     67        67.1
//   1     1     52.8     70.4      70.2

// Pippenger:
// 	1	2	3	4	5	6	7	8	9	bestN
// 2	555	598	621	804	1038	1733	2486	5020	8304	1
// 4	783	747	800	1006	1428	2132	3285	5185	9806	2
// 8	1174	1071	1095	1286	1640	2398	3869	6378	12080	2
// 16	2279	1874	1745	1739	2144	2831	4209	6964	12007	4
// 32	3910	3706	2588	2477	2782	3467	4856	7489	12618	4
// 64	7184	5429	4710	4368	4010	4672	6027	8559	13684	5
// 128	14097	10574	8452	7297	6841	6718	8615	10580	15641	6
// 256	27715	20800	16000	13550	11875	11400	11505	14090	18460	6
// 512	55100	41250	31740	26570	22030	19830	20760	21380	25215	6
// 1024	111520	79000	61080	49720	43080	38320	37600	35040	36750	8
// 2048	219480	162680	122120	102080	83760	70360	66600	63920	66160	8
// 4096	453320	323080	247240	210200	180040	150240	132440	114920	110560	9

// 			2	4	8	16	32	64	128	256	512	1024	2048	4096
// Bos Coster		858	994	1316	1949	3183	5512	9865	17830	33485	63160	124280	246320
// Straus		226	341	548	980	1870	3538	7039	14490	29020	57200	118640	233640
// Straus/cached	226	315	485	785	1514	2858	5753	11065	22970	45120	98880	194840
// Pippenger		555	747	1071	1739	2477	4010	6718	11400	19830	35040	63920	110560

// Best/cached		Straus	Straus	Straus	Straus	Straus	Straus	Straus	Straus	Pip	Pip	Pip	Pip
// Best/uncached	Straus	Straus	Straus	Straus	Straus	Straus	Pip	Pip	Pip	Pip	Pip	Pip

namespace rct
{

static inline bool operator<(const rct::key &k0, const rct::key &k1)
{
	for(int n = 31; n >= 0; --n)
	{
		if(k0.bytes[n] < k1.bytes[n])
			return true;
		if(k0.bytes[n] > k1.bytes[n])
			return false;
	}
	return false;
}

static inline rct::key div2(const rct::key &k)
{
	rct::key res;
	int carry = 0;
	for(int n = 31; n >= 0; --n)
	{
		int new_carry = (k.bytes[n] & 1) << 7;
		res.bytes[n] = k.bytes[n] / 2 + carry;
		carry = new_carry;
	}
	return res;
}

static inline rct::key pow2(size_t n)
{
	CHECK_AND_ASSERT_THROW_MES(n < 256, "Invalid pow2 argument");
	rct::key res = rct::zero();
	res[n >> 3] |= 1 << (n & 7);
	return res;
}

static inline int test(const rct::key &k, size_t n)
{
	if(n >= 256)
		return 0;
	return k[n >> 3] & (1 << (n & 7));
}

static inline void add(ge_p3 &p3, const ge_cached &other)
{
	ge_p1p1 p1;
	ge_add(&p1, &p3, &other);
	ge_p1p1_to_p3(&p3, &p1);
}

static inline void add(ge_p3 &p3, const ge_p3 &other)
{
	ge_cached cached;
	ge_p3_to_cached(&cached, &other);
	add(p3, cached);
}

rct::key bos_coster_heap_conv(std::vector<MultiexpData> data)
{
	MULTIEXP_PERF(PERF_TIMER_START_UNIT(bos_coster, 1000000));
	MULTIEXP_PERF(PERF_TIMER_START_UNIT(setup, 1000000));
	size_t points = data.size();
	CHECK_AND_ASSERT_THROW_MES(points > 1, "Not enough points");
	std::vector<size_t> heap(points);
	for(size_t n = 0; n < points; ++n)
		heap[n] = n;

	auto Comp = [&](size_t e0, size_t e1) { return data[e0].scalar < data[e1].scalar; };
	std::make_heap(heap.begin(), heap.end(), Comp);
	MULTIEXP_PERF(PERF_TIMER_STOP(setup));

	MULTIEXP_PERF(PERF_TIMER_START_UNIT(loop, 1000000));
	MULTIEXP_PERF(PERF_TIMER_START_UNIT(pop, 1000000));
	MULTIEXP_PERF(PERF_TIMER_PAUSE(pop));
	MULTIEXP_PERF(PERF_TIMER_START_UNIT(add, 1000000));
	MULTIEXP_PERF(PERF_TIMER_PAUSE(add));
	MULTIEXP_PERF(PERF_TIMER_START_UNIT(sub, 1000000));
	MULTIEXP_PERF(PERF_TIMER_PAUSE(sub));
	MULTIEXP_PERF(PERF_TIMER_START_UNIT(push, 1000000));
	MULTIEXP_PERF(PERF_TIMER_PAUSE(push));
	while(heap.size() > 1)
	{
		MULTIEXP_PERF(PERF_TIMER_RESUME(pop));
		std::pop_heap(heap.begin(), heap.end(), Comp);
		size_t index1 = heap.back();
		heap.pop_back();
		std::pop_heap(heap.begin(), heap.end(), Comp);
		size_t index2 = heap.back();
		heap.pop_back();
		MULTIEXP_PERF(PERF_TIMER_PAUSE(pop));

		MULTIEXP_PERF(PERF_TIMER_RESUME(add));
		ge_cached cached;
		ge_p3_to_cached(&cached, &data[index1].point);
		ge_p1p1 p1;
		ge_add(&p1, &data[index2].point, &cached);
		ge_p1p1_to_p3(&data[index2].point, &p1);
		MULTIEXP_PERF(PERF_TIMER_PAUSE(add));

		MULTIEXP_PERF(PERF_TIMER_RESUME(sub));
		sc_sub(data[index1].scalar.bytes, data[index1].scalar.bytes, data[index2].scalar.bytes);
		MULTIEXP_PERF(PERF_TIMER_PAUSE(sub));

		MULTIEXP_PERF(PERF_TIMER_RESUME(push));
		if(!(data[index1].scalar == rct::zero()))
		{
			heap.push_back(index1);
			std::push_heap(heap.begin(), heap.end(), Comp);
		}

		heap.push_back(index2);
		std::push_heap(heap.begin(), heap.end(), Comp);
		MULTIEXP_PERF(PERF_TIMER_PAUSE(push));
	}
	MULTIEXP_PERF(PERF_TIMER_STOP(push));
	MULTIEXP_PERF(PERF_TIMER_STOP(sub));
	MULTIEXP_PERF(PERF_TIMER_STOP(add));
	MULTIEXP_PERF(PERF_TIMER_STOP(pop));
	MULTIEXP_PERF(PERF_TIMER_STOP(loop));

	MULTIEXP_PERF(PERF_TIMER_START_UNIT(end, 1000000));
	//return rct::scalarmultKey(data[index1].point, data[index1].scalar);
	std::pop_heap(heap.begin(), heap.end(), Comp);
	size_t index1 = heap.back();
	heap.pop_back();
	ge_p2 p2;
	ge_scalarmult(&p2, data[index1].scalar.bytes, &data[index1].point);
	rct::key res;
	ge_tobytes(res.bytes, &p2);
	return res;
}

rct::key bos_coster_heap_conv_robust(std::vector<MultiexpData> data)
{
	MULTIEXP_PERF(PERF_TIMER_START_UNIT(bos_coster, 1000000));
	MULTIEXP_PERF(PERF_TIMER_START_UNIT(setup, 1000000));
	size_t points = data.size();
	CHECK_AND_ASSERT_THROW_MES(points > 0, "Not enough points");
	std::vector<size_t> heap;
	heap.reserve(points);
	for(size_t n = 0; n < points; ++n)
	{
		if(!(data[n].scalar == rct::zero()) && !ge_p3_is_point_at_infinity(&data[n].point))
			heap.push_back(n);
	}
	points = heap.size();
	if(points == 0)
		return rct::identity();

	auto Comp = [&](size_t e0, size_t e1) { return data[e0].scalar < data[e1].scalar; };
	std::make_heap(heap.begin(), heap.end(), Comp);

	if(points < 2)
	{
		std::pop_heap(heap.begin(), heap.end(), Comp);
		size_t index1 = heap.back();
		ge_p2 p2;
		ge_scalarmult(&p2, data[index1].scalar.bytes, &data[index1].point);
		rct::key res;
		ge_tobytes(res.bytes, &p2);
		return res;
	}

	MULTIEXP_PERF(PERF_TIMER_STOP(setup));

	MULTIEXP_PERF(PERF_TIMER_START_UNIT(loop, 1000000));
	MULTIEXP_PERF(PERF_TIMER_START_UNIT(pop, 1000000));
	MULTIEXP_PERF(PERF_TIMER_PAUSE(pop));
	MULTIEXP_PERF(PERF_TIMER_START_UNIT(div, 1000000));
	MULTIEXP_PERF(PERF_TIMER_PAUSE(div));
	MULTIEXP_PERF(PERF_TIMER_START_UNIT(add, 1000000));
	MULTIEXP_PERF(PERF_TIMER_PAUSE(add));
	MULTIEXP_PERF(PERF_TIMER_START_UNIT(sub, 1000000));
	MULTIEXP_PERF(PERF_TIMER_PAUSE(sub));
	MULTIEXP_PERF(PERF_TIMER_START_UNIT(push, 1000000));
	MULTIEXP_PERF(PERF_TIMER_PAUSE(push));
	while(heap.size() > 1)
	{
		MULTIEXP_PERF(PERF_TIMER_RESUME(pop));
		std::pop_heap(heap.begin(), heap.end(), Comp);
		size_t index1 = heap.back();
		heap.pop_back();
		std::pop_heap(heap.begin(), heap.end(), Comp);
		size_t index2 = heap.back();
		heap.pop_back();
		MULTIEXP_PERF(PERF_TIMER_PAUSE(pop));

		ge_cached cached;
		ge_p1p1 p1;
		ge_p2 p2;

		MULTIEXP_PERF(PERF_TIMER_RESUME(div));
		while(1)
		{
			rct::key s1_2 = div2(data[index1].scalar);
			if(!(data[index2].scalar < s1_2))
				break;
			if(data[index1].scalar.bytes[0] & 1)
			{
				data.resize(data.size() + 1);
				data.back().scalar = rct::identity();
				data.back().point = data[index1].point;
				heap.push_back(data.size() - 1);
				std::push_heap(heap.begin(), heap.end(), Comp);
			}
			data[index1].scalar = div2(data[index1].scalar);
			ge_p3_to_p2(&p2, &data[index1].point);
			ge_p2_dbl(&p1, &p2);
			ge_p1p1_to_p3(&data[index1].point, &p1);
		}
		MULTIEXP_PERF(PERF_TIMER_PAUSE(div));

		MULTIEXP_PERF(PERF_TIMER_RESUME(add));
		ge_p3_to_cached(&cached, &data[index1].point);
		ge_add(&p1, &data[index2].point, &cached);
		ge_p1p1_to_p3(&data[index2].point, &p1);
		MULTIEXP_PERF(PERF_TIMER_PAUSE(add));

		MULTIEXP_PERF(PERF_TIMER_RESUME(sub));
		sc_sub(data[index1].scalar.bytes, data[index1].scalar.bytes, data[index2].scalar.bytes);
		MULTIEXP_PERF(PERF_TIMER_PAUSE(sub));

		MULTIEXP_PERF(PERF_TIMER_RESUME(push));
		if(!(data[index1].scalar == rct::zero()))
		{
			heap.push_back(index1);
			std::push_heap(heap.begin(), heap.end(), Comp);
		}

		heap.push_back(index2);
		std::push_heap(heap.begin(), heap.end(), Comp);
		MULTIEXP_PERF(PERF_TIMER_PAUSE(push));
	}
	MULTIEXP_PERF(PERF_TIMER_STOP(push));
	MULTIEXP_PERF(PERF_TIMER_STOP(sub));
	MULTIEXP_PERF(PERF_TIMER_STOP(add));
	MULTIEXP_PERF(PERF_TIMER_STOP(pop));
	MULTIEXP_PERF(PERF_TIMER_STOP(loop));

	MULTIEXP_PERF(PERF_TIMER_START_UNIT(end, 1000000));
	//return rct::scalarmultKey(data[index1].point, data[index1].scalar);
	std::pop_heap(heap.begin(), heap.end(), Comp);
	size_t index1 = heap.back();
	heap.pop_back();
	ge_p2 p2;
	ge_scalarmult(&p2, data[index1].scalar.bytes, &data[index1].point);
	rct::key res;
	ge_tobytes(res.bytes, &p2);
	return res;
}

static rct::key get_exponent(const rct::key &base, size_t idx)
{
	static const std::string salt("bulletproof");
	std::string hashed = std::string((const char *)base.bytes, sizeof(base)) + salt + tools::get_varint_data(idx);
	const rct::key e = rct::hashToPoint(rct::hash2rct(crypto::cn_fast_hash(hashed.data(), hashed.size())));
	CHECK_AND_ASSERT_THROW_MES(!(e == rct::identity()), "Exponent is point at infinity");
	return e;
}

multiexp_cache::multiexp_cache()
{
	std::vector<MultiexpData> data;
	for(size_t i = 0; i < maxN * maxM; ++i)
	{
		Hi_cache[i] = get_exponent(rct::H, i * 2);
		CHECK_AND_ASSERT_THROW_MES(ge_frombytes_vartime(&Hi_p3_cache[i], Hi_cache[i].bytes) == 0, "ge_frombytes_vartime failed");
		Gi_cache[i] = get_exponent(rct::H, i * 2 + 1);
		CHECK_AND_ASSERT_THROW_MES(ge_frombytes_vartime(&Gi_p3_cache[i], Gi_cache[i].bytes) == 0, "ge_frombytes_vartime failed");

		data.push_back({rct::zero(), Gi_cache[i]});
		data.push_back({rct::zero(), Hi_cache[i]});
	}

	s_cache.init_cache(data, STRAUS_SIZE_LIMIT);
	p_cache.init_cache(data, 0, false);

	MINFO("Hi/Gi cache size: " << (sizeof(Hi_cache) + sizeof(Gi_cache)) / 1024 << " kB");
	MINFO("Hi_p3/Gi_p3 cache size: " << (sizeof(Hi_p3_cache) + sizeof(Gi_p3_cache)) / 1024 << " kB");
	MINFO("Straus cache size: " << sizeof(straus_cache) / 1024 << " kB");
	MINFO("Pippenger cache size: " << sizeof(pippenger_cache) / 1024 << " kB");
	size_t cache_size = (sizeof(Hi_cache) + sizeof(Hi_p3_cache)) * 2 + sizeof(straus_cache) + sizeof(pippenger_cache);
	MINFO("Total cache size: " << cache_size / 1024 << "kB");
}

void straus_cache::init_cache(const std::vector<MultiexpData> &data, size_t N)
{
	MULTIEXP_PERF(PERF_TIMER_START_UNIT(multiples, 1000000));
	if (N == 0)
		N = data.size();
	CHECK_AND_ASSERT_THROW_MES(N <= data.size(), "Bad cache base data");
	ge_p1p1 p1;
	ge_p3 p3;

	size = N;
	if(cache == nullptr || alloc_size < N)
	{
		alloc_size = std::max<size_t>(N, 64);
		cache = make_aligned_array<ge_cached_pad>(4096, ((1<<STRAUS_C)-1) * alloc_size);
	}

	for (size_t j=0;j<N;++j)
	{
		ge_p3_to_cached(st_offset(j, 1), &data[j].point);
		for (size_t i=2;i<1<<STRAUS_C;++i)
		{
			ge_add(&p1, &data[j].point, st_offset(j, i-1));
			ge_p1p1_to_p3(&p3, &p1);
			ge_p3_to_cached(st_offset(j, i), &p3);
		}
	}
	MULTIEXP_PERF(PERF_TIMER_STOP(multiples));
}

rct::key straus_cache::straus(const std::vector<MultiexpData> &data, size_t STEP) const
{
	CHECK_AND_ASSERT_THROW_MES(cache != nullptr, "null cache");
	CHECK_AND_ASSERT_THROW_MES(cache == NULL || size >= data.size(), "Cache is too small");
	MULTIEXP_PERF(PERF_TIMER_UNIT(straus, 1000000));
	STEP = STEP ? STEP : 192;

	MULTIEXP_PERF(PERF_TIMER_START_UNIT(setup, 1000000));
	static constexpr unsigned int mask = (1 << STRAUS_C) - 1;
	
	ge_cached cached;
	ge_p1p1 p1;
	ge_p3 p3;

#ifdef TRACK_STRAUS_ZERO_IDENTITY
	MULTIEXP_PERF(PERF_TIMER_START_UNIT(skip, 1000000));
	std::vector<uint8_t> skip(data.size());
	for(size_t i = 0; i < data.size(); ++i)
		skip[i] = data[i].scalar == rct::zero() || ge_p3_is_point_at_infinity(&data[i].point);
	MULTIEXP_PERF(PERF_TIMER_STOP(skip));
#endif

	MULTIEXP_PERF(PERF_TIMER_START_UNIT(digits, 1000000));
	std::unique_ptr<uint8_t[]> digits{new uint8_t[256 * data.size()]};
	for(size_t j = 0; j < data.size(); ++j)
	{
		unsigned char bytes33[33];
		memcpy(bytes33, data[j].scalar.bytes, 32);
		bytes33[32] = 0;
		const unsigned char *bytes = bytes33;
		static_assert(STRAUS_C == 4, "optimized version needs STRAUS_C == 4");
		unsigned int i;
		for(i = 0; i < 256; i += 8, bytes++)
		{
			digits[j * 256 + i] = bytes[0] & 0xf;
			digits[j * 256 + i + 1] = (bytes[0] >> 1) & 0xf;
			digits[j * 256 + i + 2] = (bytes[0] >> 2) & 0xf;
			digits[j * 256 + i + 3] = (bytes[0] >> 3) & 0xf;
			digits[j * 256 + i + 4] = ((bytes[0] >> 4) | (bytes[1] << 4)) & 0xf;
			digits[j * 256 + i + 5] = ((bytes[0] >> 5) | (bytes[1] << 3)) & 0xf;
			digits[j * 256 + i + 6] = ((bytes[0] >> 6) | (bytes[1] << 2)) & 0xf;
			digits[j * 256 + i + 7] = ((bytes[0] >> 7) | (bytes[1] << 1)) & 0xf;
		}
	}
	MULTIEXP_PERF(PERF_TIMER_STOP(digits));

	rct::key maxscalar = rct::zero();
	for(size_t i = 0; i < data.size(); ++i)
		if(maxscalar < data[i].scalar)
			maxscalar = data[i].scalar;
	size_t start_i = 0;
	while(start_i < 256 && !(maxscalar < pow2(start_i)))
		start_i += STRAUS_C;
	MULTIEXP_PERF(PERF_TIMER_STOP(setup));

	ge_p3 res_p3 = ge_p3_identity;

	for(size_t start_offset = 0; start_offset < data.size(); start_offset += STEP)
	{
		const size_t num_points = std::min(data.size() - start_offset, STEP);

		ge_p3 band_p3 = ge_p3_identity;
		size_t i = start_i;
		if(!(i < STRAUS_C))
			goto skipfirst;
		while(!(i < STRAUS_C))
		{
			ge_p2 p2;
			ge_p3_to_p2(&p2, &band_p3);
			for(size_t j = 0; j < STRAUS_C; ++j)
			{
				ge_p2_dbl(&p1, &p2);
				if(j == STRAUS_C - 1)
					ge_p1p1_to_p3(&band_p3, &p1);
				else
					ge_p1p1_to_p2(&p2, &p1);
			}
		skipfirst:
			i -= STRAUS_C;
			for(size_t j = start_offset; j < start_offset + num_points; ++j)
			{
#ifdef TRACK_STRAUS_ZERO_IDENTITY
				if(skip[j])
					continue;
#endif
				const uint8_t digit = digits[j * 256 + i];
				if(digit)
				{
					ge_add(&p1, &band_p3, &st_offset(j, digit));
					ge_p1p1_to_p3(&band_p3, &p1);
				}
			}
		}

		ge_p3_to_cached(&cached, &band_p3);
		ge_add(&p1, &res_p3, &cached);
		ge_p1p1_to_p3(&res_p3, &p1);
	}

	rct::key res;
	ge_p3_tobytes(res.bytes, &res_p3);
	return res;
}

void pippenger_cache::init_cache(const std::vector<MultiexpData> &data, size_t N, bool over_alloc)
{
	MULTIEXP_PERF(PERF_TIMER_START_UNIT(pippenger_init_cache, 1000000));
	if(N == 0)
		N = data.size();
	CHECK_AND_ASSERT_THROW_MES(N <= data.size(), "Bad cache base data");

	size = N;
	if(cache == nullptr || alloc_size < N)
	{
		if(over_alloc)
			alloc_size = 2*N;
		else
			alloc_size = N;

		cache = make_aligned_array<ge_cached_pad>(4096, alloc_size);
	}

	for(size_t i = 0; i < N; ++i)
		ge_p3_to_cached(pp_offset(i), &data[i].point);

	MULTIEXP_PERF(PERF_TIMER_STOP(pippenger_init_cache));
}

rct::key pippenger_cache::pippenger(const std::vector<MultiexpData> &data, aligned_ptr<ge_p3[]>& buckets, size_t c) const
{
	CHECK_AND_ASSERT_THROW_MES(cache != nullptr, "null cache");
	CHECK_AND_ASSERT_THROW_MES(size >= data.size(), "Cache is too small");

	if(c == 0)
		c = get_pippenger_c(data.size());

	CHECK_AND_ASSERT_THROW_MES(c <= 9, "c is too large");

	ge_p3 result = ge_p3_identity;

	if(buckets == nullptr)
		buckets = make_aligned_array<ge_p3>(4096, 1 << 9);

	rct::key maxscalar = rct::zero();
	for(size_t i = 0; i < data.size(); ++i)
	{
		if(maxscalar < data[i].scalar)
			maxscalar = data[i].scalar;
	}

	size_t groups = 0;
	while(groups < 256 && !(maxscalar < pow2(groups)))
		++groups;
	groups = (groups + c - 1) / c;

	for(size_t k = groups; k-- > 0;)
	{
		if(!ge_p3_is_point_at_infinity(&result))
		{
			ge_p2 p2;
			ge_p3_to_p2(&p2, &result);
			for(size_t i = 0; i < c; ++i)
			{
				ge_p1p1 p1;
				ge_p2_dbl(&p1, &p2);
				if(i == c - 1)
					ge_p1p1_to_p3(&result, &p1);
				else
					ge_p1p1_to_p2(&p2, &p1);
			}
		}
		for(size_t i = 0; i < (1u << c); ++i)
			buckets[i] = ge_p3_identity;

		// partition scalars into buckets
		for(size_t i = 0; i < data.size(); ++i)
		{
			unsigned int bucket = 0;
			for(size_t j = 0; j < c; ++j)
				if(test(data[i].scalar, k * c + j))
					bucket |= 1 << j;
			if(bucket == 0)
				continue;
			CHECK_AND_ASSERT_THROW_MES(bucket < (1u << c), "bucket overflow");
			if(!ge_p3_is_point_at_infinity(&buckets[bucket]))
			{
				add(buckets[bucket], pp_offset(i));
			}
			else
				buckets[bucket] = data[i].point;
		}

		// sum the buckets
		ge_p3 pail = ge_p3_identity;
		for(size_t i = (1 << c) - 1; i > 0; --i)
		{
			if(!ge_p3_is_point_at_infinity(&buckets[i]))
				add(pail, buckets[i]);
			if(!ge_p3_is_point_at_infinity(&pail))
				add(result, pail);
		}
	}

	rct::key res;
	ge_p3_tobytes(res.bytes, &result);
	return res;
}

/* Given two scalar arrays, construct a vector commitment */
rct::key bp_cache::vector_exponent(const rct::keyV &a, const rct::keyV &b)
{
	CHECK_AND_ASSERT_THROW_MES(a.size() == b.size(), "Incompatible sizes of a and b");
	CHECK_AND_ASSERT_THROW_MES(a.size() <= maxN * maxM, "Incompatible sizes of a and maxN");

	clear_pad(a.size()*2);
	for(size_t i = 0; i < a.size(); ++i)
	{
		me_pad.emplace_back(a[i], multiexp_cache::Gi_p3(i));
		me_pad.emplace_back(b[i], multiexp_cache::Hi_p3(i));
	}

	return multiexp_higi();
}

/* Compute a custom vector-scalar commitment */
rct::key bp_cache::vector_exponent_custom(const rct::keyV &A, const rct::keyV &B, const rct::keyV &a, const rct::keyV &b)
{
	CHECK_AND_ASSERT_THROW_MES(A.size() == B.size(), "Incompatible sizes of A and B");
	CHECK_AND_ASSERT_THROW_MES(a.size() == b.size(), "Incompatible sizes of a and b");
	CHECK_AND_ASSERT_THROW_MES(a.size() == A.size(), "Incompatible sizes of a and A");
	CHECK_AND_ASSERT_THROW_MES(a.size() <= maxN * maxM, "Incompatible sizes of a and maxN");

	clear_pad(a.size()*2);
	for(size_t i = 0; i < a.size(); ++i)
	{
		me_pad.emplace_back(a[i], A[i]);
		me_pad.emplace_back(b[i], B[i]);
	}

	return multiexp();
}
}

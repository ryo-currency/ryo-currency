// Copyright (c) 2017, SUMOKOIN
// Copyright (c) 2014-2016, The Monero Project
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
//    public domain on 1st of February 2021
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

#include "common/gulps.hpp"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "common/int-util.h"
#include "crypto/hash.h"
#include "cryptonote_config.h"
#include "difficulty.h"
#include "include_base_utils.h"
#include "misc_language.h"

GULPS_CAT_MAJOR("crybas_diff");

namespace cryptonote
{

using std::size_t;
using std::uint64_t;
using std::vector;

#if defined(__x86_64__)
static inline void mul(uint64_t a, uint64_t b, uint64_t &low, uint64_t &high)
{
	low = mul128(a, b, &high);
}

#else

static inline void mul(uint64_t a, uint64_t b, uint64_t &low, uint64_t &high)
{
	// __int128 isn't part of the standard, so the previous function wasn't portable. mul128() in Windows is fine,
	// but this portable function should be used elsewhere. Credit for this function goes to latexi95.

	uint64_t aLow = a & 0xFFFFFFFF;
	uint64_t aHigh = a >> 32;
	uint64_t bLow = b & 0xFFFFFFFF;
	uint64_t bHigh = b >> 32;

	uint64_t res = aLow * bLow;
	uint64_t lowRes1 = res & 0xFFFFFFFF;
	uint64_t carry = res >> 32;

	res = aHigh * bLow + carry;
	uint64_t highResHigh1 = res >> 32;
	uint64_t highResLow1 = res & 0xFFFFFFFF;

	res = aLow * bHigh;
	uint64_t lowRes2 = res & 0xFFFFFFFF;
	carry = res >> 32;

	res = aHigh * bHigh + carry;
	uint64_t highResHigh2 = res >> 32;
	uint64_t highResLow2 = res & 0xFFFFFFFF;

	//Addition

	uint64_t r = highResLow1 + lowRes2;
	carry = r >> 32;
	low = (r << 32) | lowRes1;
	r = highResHigh1 + highResLow2 + carry;
	uint64_t d3 = r & 0xFFFFFFFF;
	carry = r >> 32;
	r = highResHigh2 + carry;
	high = d3 | (r << 32);
}

#endif

static inline bool cadd(uint64_t a, uint64_t b)
{
	return a + b < a;
}

static inline bool cadc(uint64_t a, uint64_t b, bool c)
{
	return a + b < a || (c && a + b == (uint64_t)-1);
}

bool check_hash(const crypto::hash &hash, difficulty_type difficulty)
{
	uint64_t low, high, top, cur;
	// First check the highest word, this will most likely fail for a random hash.
	mul(swap64le(((const uint64_t *)&hash)[3]), difficulty, top, high);
	if(high != 0)
	{
		return false;
	}
	mul(swap64le(((const uint64_t *)&hash)[0]), difficulty, low, cur);
	mul(swap64le(((const uint64_t *)&hash)[1]), difficulty, low, high);
	bool carry = cadd(cur, low);
	cur = high;
	mul(swap64le(((const uint64_t *)&hash)[2]), difficulty, low, high);
	carry = cadc(cur, low, carry);
	carry = cadc(high, top, carry);
	return !carry;
}

difficulty_type next_difficulty_v1(std::vector<std::uint64_t> timestamps, std::vector<difficulty_type> cumulative_difficulties, size_t target_seconds)
{
	if(timestamps.size() > common_config::DIFFICULTY_WINDOW_V1)
	{
		timestamps.resize(common_config::DIFFICULTY_WINDOW_V1);
		cumulative_difficulties.resize(common_config::DIFFICULTY_WINDOW_V1);
	}

	size_t length = timestamps.size();
	assert(length == cumulative_difficulties.size());
	if(length <= 1)
	{
		return 1;
	}
	static_assert(common_config::DIFFICULTY_WINDOW_V1 >= 2, "Window is too small");
	assert(length <= common_config::DIFFICULTY_WINDOW_V1);
	sort(timestamps.begin(), timestamps.end());
	size_t cut_begin, cut_end;
	static_assert(2 * common_config::DIFFICULTY_CUT_V1 <= common_config::DIFFICULTY_WINDOW_V1 - 2, "Cut length is too large");
	if(length <= common_config::DIFFICULTY_WINDOW_V1 - 2 * common_config::DIFFICULTY_CUT_V1)
	{
		cut_begin = 0;
		cut_end = length;
	}
	else
	{
		cut_begin = (length - (common_config::DIFFICULTY_WINDOW_V1 - 2 * common_config::DIFFICULTY_CUT_V1) + 1) / 2;
		cut_end = cut_begin + (common_config::DIFFICULTY_WINDOW_V1 - 2 * common_config::DIFFICULTY_CUT_V1);
	}
	assert(/*cut_begin >= 0 &&*/ cut_begin + 2 <= cut_end && cut_end <= length);
	uint64_t time_span = timestamps[cut_end - 1] - timestamps[cut_begin];
	if(time_span == 0)
	{
		time_span = 1;
	}
	difficulty_type total_work = cumulative_difficulties[cut_end - 1] - cumulative_difficulties[cut_begin];
	assert(total_work > 0);
	uint64_t low, high;
	mul(total_work, target_seconds, low, high);
	// blockchain errors "difficulty overhead" if this function returns zero.
	// TODO: consider throwing an exception instead
	if(high != 0 || low + time_span - 1 < low)
	{
		return 0;
	}
	return (low + time_span - 1) / time_span;
}

difficulty_type next_difficulty_v2(std::vector<std::uint64_t> timestamps, std::vector<difficulty_type> cumulative_difficulties, size_t target_seconds)
{
	constexpr uint64_t MAX_AVERAGE_TIMESPAN = common_config::DIFFICULTY_TARGET * 6;  // 24 minutes
	constexpr uint64_t MIN_AVERAGE_TIMESPAN = common_config::DIFFICULTY_TARGET / 24; // 10s

	if(timestamps.size() > common_config::DIFFICULTY_BLOCKS_COUNT_V2)
	{
		timestamps.resize(common_config::DIFFICULTY_BLOCKS_COUNT_V2);
		cumulative_difficulties.resize(common_config::DIFFICULTY_BLOCKS_COUNT_V2);
	}

	size_t length = timestamps.size();
	assert(length == cumulative_difficulties.size());
	if(length <= 1)
	{
		return 1;
	}

	sort(timestamps.begin(), timestamps.end());
	size_t cut_begin, cut_end;
	static_assert(2 * common_config::DIFFICULTY_CUT_V2 <= common_config::DIFFICULTY_BLOCKS_COUNT_V2 - 2, "Cut length is too large");
	if(length <= common_config::DIFFICULTY_BLOCKS_COUNT_V2 - 2 * common_config::DIFFICULTY_CUT_V2)
	{
		cut_begin = 0;
		cut_end = length;
	}
	else
	{
		cut_begin = (length - (common_config::DIFFICULTY_BLOCKS_COUNT_V2 - 2 * common_config::DIFFICULTY_CUT_V2) + 1) / 2;
		cut_end = cut_begin + (common_config::DIFFICULTY_BLOCKS_COUNT_V2 - 2 * common_config::DIFFICULTY_CUT_V2);
	}
	assert(/*cut_begin >= 0 &&*/ cut_begin + 2 <= cut_end && cut_end <= length);
	uint64_t total_timespan = timestamps[cut_end - 1] - timestamps[cut_begin];
	if(total_timespan == 0)
	{
		total_timespan = 1;
	}

	uint64_t timespan_median = 0;
	if(cut_begin > 0 && length >= cut_begin * 2 + 3)
	{
		std::vector<std::uint64_t> time_spans;
		for(size_t i = length - cut_begin * 2 - 3; i < length - 1; i++)
		{
			uint64_t time_span = timestamps[i + 1] - timestamps[i];
			if(time_span == 0)
			{
				time_span = 1;
			}
			time_spans.push_back(time_span);

			GULPSF_LOG_L3("Timespan {}: {}:{}:{} ({})", i, (time_span / 60) / 60, (time_span > 3600 ? (time_span % 3600) / 60 : time_span / 60), time_span % 60, time_span );

		}
		timespan_median = epee::misc_utils::median(time_spans);
	}

	uint64_t timespan_length = length - cut_begin * 2 - 1;
	GULPSF_LOG_L2("Timespan Median: {}, Timespan Average: {}", timespan_median, total_timespan / timespan_length);

	uint64_t total_timespan_median = timespan_median > 0 ? timespan_median * timespan_length : total_timespan * 7 / 10;
	uint64_t adjusted_total_timespan = (total_timespan * 8 + total_timespan_median * 3) / 10; //  0.8A + 0.3M (the median of a poisson distribution is 70% of the mean, so 0.25A = 0.25/0.7 = 0.285M)
	if(adjusted_total_timespan > MAX_AVERAGE_TIMESPAN * timespan_length)
	{
		adjusted_total_timespan = MAX_AVERAGE_TIMESPAN * timespan_length;
	}
	if(adjusted_total_timespan < MIN_AVERAGE_TIMESPAN * timespan_length)
	{
		adjusted_total_timespan = MIN_AVERAGE_TIMESPAN * timespan_length;
	}

	difficulty_type total_work = cumulative_difficulties[cut_end - 1] - cumulative_difficulties[cut_begin];
	assert(total_work > 0);

	uint64_t low, high;
	mul(total_work, target_seconds, low, high);
	if(high != 0)
	{
		return 0;
	}

	uint64_t next_diff = (low + adjusted_total_timespan - 1) / adjusted_total_timespan;
	if(next_diff < 1)
		next_diff = 1;
	GULPSF_LOG_L2("Total timespan: {}, Adjusted total timespan: {}, Total work: {}, Next diff: {}, Hashrate (H/s): {}", total_timespan, adjusted_total_timespan, total_work, next_diff, next_diff / target_seconds);
	return next_diff;
}

template <typename T>
inline T clamp(T lo, T v, T hi)
{
	return v < lo ? lo : v > hi ? hi : v;
}

// LWMA difficulty algorithm
// Copyright (c) 2017-2018 Zawy
// BSD-3 Licensed
// https://github.com/zawy12/difficulty-algorithms/issues/3

difficulty_type next_difficulty_v3(const std::vector<std::uint64_t> &timestamps, const std::vector<difficulty_type> &cumulative_difficulties)
{
	constexpr int64_t T = common_config::DIFFICULTY_TARGET;
	constexpr int64_t N = common_config::DIFFICULTY_WINDOW_V3;
	constexpr int64_t FTL = common_config::BLOCK_FUTURE_TIME_LIMIT_V3;

	assert(timestamps.size() == N + 1);
	assert(cumulative_difficulties.size() == N + 1);

	int64_t L = 0;
	for(int64_t i = 1; i <= N; i++)
		L += clamp(-FTL, int64_t(timestamps[i]) - int64_t(timestamps[i - 1]), 6 * T) * i;

	constexpr int64_t clamp_increase = (T * N * (N + 1) * 99) / int64_t(100.0 * 2.0 * 2.5);
	constexpr int64_t clamp_decrease = (T * N * (N + 1) * 99) / int64_t(100.0 * 2.0 * 0.2);
	L = clamp(clamp_increase, L, clamp_decrease); // This guarantees positive L

	// Commetary by fireice
	// Let's take CD as a sum of N difficulties. Sum of weights is 0.5(N^2+N)
	// L is a sigma(timeperiods * weights)
	// Therefore D = CD*0.5*(N^2+N)*T / NL
	// Therefore D = 0.5*CD*(N+1)*T / L
	// Therefore D = CD*(N+1)*T / 2L
	uint64_t next_D = uint64_t((cumulative_difficulties[N] - cumulative_difficulties[0]) * T * (N + 1)) / uint64_t(2 * L);

	// 99/100 adds a small bias towards decreasing diff, unlike zawy we do it in a separate step to avoid an overflow at 6GH/s
	next_D = (next_D * 99ull) / 100ull;

	GULPSF_LOG_L2("diff sum: {} L {} sizes {} {} next_D {}",(cumulative_difficulties[N] - cumulative_difficulties[0]), L, timestamps.size(), cumulative_difficulties.size(), next_D);
	return next_D;
}

//Find non-zero timestamp with index smaller than i
inline uint64_t findLastValid(const std::vector<uint64_t>& timestamps, size_t i)
{
	while(--i >= 1)
	{
		if(timestamps[i] != 0)
			return timestamps[i];
	}

	return timestamps[0];
}

template<size_t N>
void interpolate_timestamps(std::vector<uint64_t>& timestamps)
{
	uint64_t maxValid = timestamps[N];
	for(size_t i = 1; i < N; i++)
	{
		/*
		 * Mask timestamp if it is smaller or equal to last valid timestamp
		 * or if it is larger or equal to largest timestamp
		 */
		if(timestamps[i] <= findLastValid(timestamps, i) || timestamps[i] >= maxValid)
		{
			if(i != 1)
				timestamps[i-1] = 0;
			timestamps[i] = 0;
		}
	}

	// Now replace zeros with number of masked timestamps before this one (inclusive)
	uint64_t mctr = 0;
	for(size_t i = 1; i < N; i++)
	{
		if(timestamps[i] == 0)
			timestamps[i] = ++mctr;
		else
			mctr = 0;
	}

	// interpolate timestamps of masked times
	for(uint64_t i = N-1; i > 0; i--)
	{
		if(timestamps[i] <= N)
		{
			// denominator -- NOT THE SAME AS [i+1]
			uint64_t den = timestamps[i] + 1;
			// numerator
			uint64_t num = timestamps[i];
			uint64_t delta = timestamps[i+1] - timestamps[i-num];

			timestamps[i] = timestamps[i-num] + (delta * num) / den;
		}
	}
}

template void interpolate_timestamps<4>(std::vector<uint64_t>& timestamps);
template void interpolate_timestamps<5>(std::vector<uint64_t>& timestamps);
template void interpolate_timestamps<7>(std::vector<uint64_t>& timestamps);

difficulty_type next_difficulty_v4(std::vector<uint64_t> timestamps, const std::vector<difficulty_type>& cumulative_difficulties)
{
	constexpr uint64_t T = common_config::DIFFICULTY_TARGET;
	constexpr uint64_t N = common_config::DIFFICULTY_WINDOW_V4;

	assert(timestamps.size() == N + 1);
	assert(cumulative_difficulties.size() == N + 1);

	// the newest timestamp is not allwed to be older than the previous
	uint64_t t_last = std::max(timestamps[N], timestamps[N - 1]);
	// the newest timestamp can only be 5 target times in the future
	timestamps[N] = std::min(t_last, timestamps[N - 1] + 5 * T);
	// highest timestamp must higher than the lowest
	timestamps[N] = std::max(timestamps[N], timestamps[0] + 1);

	interpolate_timestamps<N>(timestamps);

	uint64_t L = 0;
	for(uint64_t i = 1; i <= N; i++)
	{
		assert(timestamps[i] >= timestamps[i - 1]);
		assert(timestamps[i] != 0);
		L += std::min(timestamps[i] - timestamps[i - 1], 5 * T) * i * i;
	}
	L = std::max<uint64_t>(L, 1);

	// Let's take CD as a sum of N difficulties. Sum of weights is (n*(n+1)*(2n+1))/6 (SUM)
	// L is a sigma(timeperiods * weights)
	// D = CD*T*SUM / NL
	// D = CD*T*N*(N+1)*(2N+1) / 6NL
	// D = CD*T*(N+1)*(2N+1) / 6L
	// TSUM = T*(N+1)*(2N+1) / 6 (const)
	// D = CD*TSUM / L

	// By a happy accident most time units are a multiple of 6 so we can prepare a TSUM without loosing accuracy
	constexpr uint64_t TSUM = (T * (N + 1) * (2 * N + 1)) / 6;
	uint64_t next_D = ((cumulative_difficulties[N] - cumulative_difficulties[0]) * TSUM) / L;

	// Sanity limits
	uint64_t prev_D = cumulative_difficulties[N] - cumulative_difficulties[N - 1];
	next_D = std::max((prev_D * 67) / 100, std::min(next_D, (prev_D * 150) / 100));

	return next_D;
}
}

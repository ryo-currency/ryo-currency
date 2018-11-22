// Copyright (c) 2018, Ryo Currency Project
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
//    public domain on 1st of February 2019
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

#include "gtest/gtest.h"
#include "cryptonote_basic/difficulty.h"

#define ASSERT_ARRAYS(x, y) \
	ASSERT_EQ(x.size(), y.size()) << "Vectors x and y are of unequal length"; \
	for (int i = 0; i < x.size(); ++i) \
		EXPECT_EQ(x[i], y[i]) << "Vectors x and y differ at index " << i;

TEST(timestamp_interpolation, null)
{
	std::vector<uint64_t> ts     = { 100, 200, 300, 400, 500 };
	std::vector<uint64_t> ts_exp = { 100, 200, 300, 400, 500 };
	
	cryptonote::interpolate_timestamps<4>(ts);
	ASSERT_ARRAYS(ts, ts_exp);
}

TEST(timestamp_interpolation, one)
{
	std::vector<uint64_t> ts     = { 100, 600, 100, 400, 500 };
	std::vector<uint64_t> ts_exp = { 100, 200, 300, 400, 500 };

	cryptonote::interpolate_timestamps<4>(ts);
	ASSERT_ARRAYS(ts, ts_exp);
}

TEST(timestamp_interpolation, two)
{
	std::vector<uint64_t> ts     = { 100, 600, 100,  50, 500 };
	std::vector<uint64_t> ts_exp = { 100, 200, 300, 400, 500 };
	
	cryptonote::interpolate_timestamps<4>(ts);
	ASSERT_ARRAYS(ts, ts_exp);
}

TEST(timestamp_interpolation, three)
{
	std::vector<uint64_t> ts     = { 100, 600, 100,  50,  70, 600 };
	std::vector<uint64_t> ts_exp = { 100, 200, 300, 400, 500, 600 };
	
	cryptonote::interpolate_timestamps<5>(ts);
	ASSERT_ARRAYS(ts, ts_exp);
}

TEST(timestamp_interpolation, three_1)
{
	std::vector<uint64_t> ts     = { 100, 200, 300, 150, 150, 600 };
	std::vector<uint64_t> ts_exp = { 100, 200, 300, 400, 500, 600 };
	
	cryptonote::interpolate_timestamps<5>(ts);
	ASSERT_ARRAYS(ts, ts_exp);
}

TEST(timestamp_interpolation, four)
{
	std::vector<uint64_t> ts     = { 100, 600, 100, 400, 500,  50,  70, 800 };
	std::vector<uint64_t> ts_exp = { 100, 200, 300, 400, 500, 600, 700, 800 };
	
	cryptonote::interpolate_timestamps<7>(ts);
	ASSERT_ARRAYS(ts, ts_exp);
}

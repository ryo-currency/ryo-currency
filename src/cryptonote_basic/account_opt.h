// Copyright (c) 2020, Ryo Currency Project
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

#include <inttypes.h>

struct acc_options
{
	static constexpr uint8_t ACC_OPT_UNKNOWN = 0;
	static constexpr uint8_t ACC_OPT_KURZ_ADDRESS = 1;
	static constexpr uint8_t ACC_OPT_LONG_ADDRESS = 2;

	static constexpr uint64_t BC_HEIGHT_PERIOD = 100000;
	static constexpr uint64_t BC_HEIGHT_ZERO_V = 300000;

	acc_options() : acc_opt_val(0) {}

	acc_options(uint8_t keying_type) : acc_opt_val(0)
	{
		set_keying_type(keying_type);
	}

	acc_options(uint8_t keying_type, uint64_t bc_height) : acc_opt_val(0)
	{
		set_keying_type(keying_type);
		set_seed_restore_height(bc_height);
	}

	uint8_t get_keying_type() const
	{
		if(acc_opt_val == 0)
			return ACC_OPT_UNKNOWN;
		else if((acc_opt_val & 3) == 1)
			return ACC_OPT_KURZ_ADDRESS;
		else if((acc_opt_val & 3) == 2)
			return ACC_OPT_LONG_ADDRESS;
		else
			return ACC_OPT_UNKNOWN;
	}

	void set_keying_type(uint8_t type)
	{
		acc_opt_val &= 0xfc;
		if(type == ACC_OPT_KURZ_ADDRESS)
			acc_opt_val |= 0x01;
		else if(type == ACC_OPT_LONG_ADDRESS)
			acc_opt_val |= 0x02;
	}

	void set_seed_restore_height(uint64_t bc_height)
	{
		if(bc_height <= BC_HEIGHT_ZERO_V)
			return;

		bc_height -= BC_HEIGHT_ZERO_V;
		bc_height /= BC_HEIGHT_PERIOD;
		bc_height = std::min<uint64_t>(bc_height, 15); // 4 bits
		acc_opt_val |= bc_height << 4;
	}

	uint64_t get_seed_restore_height()
	{
		uint64_t height = acc_opt_val >> 4;
		if(height == 0)
			return 0;
		else
			return height * BC_HEIGHT_PERIOD + BC_HEIGHT_ZERO_V;
	}

	uint8_t get_opt_value() const
	{
		return acc_opt_val;
	}

	void set_opt_value(uint8_t v)
	{
		acc_opt_val = v;
	}

private:
	uint8_t acc_opt_val = 0;
};

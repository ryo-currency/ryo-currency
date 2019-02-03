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

#include "log.hpp"
#include "misc_log_ex.h"

namespace hw
{

#ifdef WITH_DEVICE_LEDGER
namespace ledger
{

//#undef RYO_DEFAULT_LOG_CATEGORY
//#define RYO_DEFAULT_LOG_CATEGORY "device.ledger"

void buffer_to_str(char *to_buff, size_t to_len, const char *buff, size_t len)
{
	CHECK_AND_ASSERT_THROW_MES(to_len > (len * 2), "destination buffer too short. At least" << (len * 2 + 1) << " bytes required");
	for(size_t i = 0; i < len; i++)
	{
		sprintf(to_buff + 2 * i, "%.02x", (unsigned char)buff[i]);
	}
}

void log_hexbuffer(std::string msg, const char *buff, size_t len)
{
	char logstr[1025];
	buffer_to_str(logstr, sizeof(logstr), buff, len);
	MDEBUG(msg << ": " << logstr);
}

void log_message(std::string msg, std::string info)
{
	MDEBUG(msg << ": " << info);
}

#ifdef DEBUG_HWDEVICE
extern crypto::secret_key dbg_viewkey;
extern crypto::secret_key dbg_spendkey;

void decrypt(char *buf, size_t len)
{
#ifdef IODUMMYCRYPT_HWDEVICE
	size_t i;
	if(len == 32)
	{
		//view key?
		for(i = 0; i < 32; i++)
		{
			if(buf[i] != 0)
				break;
		}
		if(i == 32)
		{
			memmove(buf, hw::ledger::dbg_viewkey.data, 32);
			return;
		}
		//spend key?
		for(i = 0; i < 32; i++)
		{
			if(buf[i] != (char)0xff)
				break;
		}
		if(i == 32)
		{
			memmove(buf, hw::ledger::dbg_spendkey.data, 32);
			return;
		}
	}
	//std decrypt: XOR.55h
	for(i = 0; i < len; i++)
	{
		buf[i] ^= 0x55;
	}
#endif
}

crypto::key_derivation decrypt(const crypto::key_derivation &derivation)
{
	crypto::key_derivation x = derivation;
	decrypt(x.data, 32);
	return x;
}

cryptonote::account_keys decrypt(const cryptonote::account_keys &keys)
{
	cryptonote::account_keys x = keys;
	decrypt(x.m_view_secret_key.data, 32);
	decrypt(x.m_spend_secret_key.data, 32);
	return x;
}

crypto::secret_key decrypt(const crypto::secret_key &sec)
{
	crypto::secret_key x = sec;
	decrypt(x.data, 32);
	return x;
}

rct::key decrypt(const rct::key &sec)
{
	rct::key x = sec;
	decrypt((char *)x.bytes, 32);
	return x;
}

crypto::ec_scalar decrypt(const crypto::ec_scalar &res)
{
	crypto::ec_scalar x = res;
	decrypt((char *)x.data, 32);
	return x;
}

rct::keyV decrypt(const rct::keyV &keys)
{
	rct::keyV x;
	for(unsigned int j = 0; j < keys.size(); j++)
	{
		x.push_back(decrypt(keys[j]));
	}
	return x;
}

static void check(std::string msg, std::string info, const char *h, const char *d, int len, bool crypted)
{
	char dd[32];
	char logstr[128];

	memmove(dd, d, len);
	if(crypted)
	{
		CHECK_AND_ASSERT_THROW_MES(len <= 32, "encrypted data greater than 32");
		decrypt(dd, len);
	}

	if(memcmp(h, dd, len))
	{
		log_message("ASSERT EQ FAIL", msg + ": " + info);
		log_hexbuffer("    host  ", h, len);
		log_hexbuffer("    device", dd, len);
	}
	else
	{
		buffer_to_str(logstr, 128, dd, len);
		log_message("ASSERT EQ OK", msg + ": " + info + ": " + std::string(logstr));
	}
}

void check32(std::string msg, std::string info, const char *h, const char *d, bool crypted)
{
	check(msg, info, h, d, 32, crypted);
}

void check8(std::string msg, std::string info, const char *h, const char *d, bool crypted)
{
	check(msg, info, h, d, 8, crypted);
}
#endif
}
#endif //WITH_DEVICE_LEDGER
}

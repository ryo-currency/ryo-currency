// Copyright (c) 2018, Ryo Currency Project
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
//

#pragma once

#include <cstddef>
#include <string>

#include "crypto/crypto.h"
#include "cryptonote_basic/account.h"
#include "ringct/rctOps.h"

#include "device.hpp"

namespace hw
{

#ifdef WITH_DEVICE_LEDGER
namespace ledger
{

void buffer_to_str(char *to_buff, size_t to_len, const char *buff, size_t len);
void log_hexbuffer(std::string msg, const char *buff, size_t len);
void log_message(std::string msg, std::string info);
#ifdef DEBUG_HWDEVICE
#define TRACK printf("file %s:%d\n", __FILE__, __LINE__)
//#define TRACK MCDEBUG("ledger"," At file " << __FILE__ << ":" << __LINE__)
//#define TRACK while(0);

void decrypt(char *buf, size_t len);
crypto::key_derivation decrypt(const crypto::key_derivation &derivation);
cryptonote::account_keys decrypt(const cryptonote::account_keys &keys);
crypto::secret_key decrypt(const crypto::secret_key &sec);
rct::key decrypt(const rct::key &sec);
crypto::ec_scalar decrypt(const crypto::ec_scalar &res);
rct::keyV decrypt(const rct::keyV &keys);

void check32(std::string msg, std::string info, const char *h, const char *d, bool crypted = false);
void check8(std::string msg, std::string info, const char *h, const char *d, bool crypted = false);

void set_check_verbose(bool verbose);
#endif
}
#endif
}

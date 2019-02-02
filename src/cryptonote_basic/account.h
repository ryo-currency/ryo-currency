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

#include "crypto/crypto.h"
#include "cryptonote_basic.h"
#include "serialization/keyvalue_serialization.h"

namespace cryptonote
{
static constexpr uint8_t ACC_OPT_UNKNOWN = 0;
static constexpr uint8_t ACC_OPT_KURZ_ADDRESS = 1;
static constexpr uint8_t ACC_OPT_LONG_ADDRESS = 2;

struct account_keys
{
	account_public_address m_account_address;
	crypto::secret_key m_spend_secret_key;
	crypto::secret_key m_view_secret_key;
	std::vector<crypto::secret_key> m_multisig_keys;
	crypto::secret_key_16 m_short_seed;
	hw::device *m_device = &hw::get_device("default");

	BEGIN_KV_SERIALIZE_MAP()
	KV_SERIALIZE(m_account_address)
	KV_SERIALIZE_VAL_POD_AS_BLOB_FORCE(m_spend_secret_key)
	KV_SERIALIZE_VAL_POD_AS_BLOB_FORCE(m_view_secret_key)
	KV_SERIALIZE_CONTAINER_POD_AS_BLOB(m_multisig_keys)
	KV_SERIALIZE_VAL_POD_AS_BLOB_FORCE(m_short_seed)
	END_KV_SERIALIZE_MAP()

	account_keys &operator=(account_keys const &) = default;

	hw::device &get_device() const;
	void set_device(hw::device &hwdev);
};

/************************************************************************/
/*                                                                      */
/************************************************************************/
class account_base
{
  public:
	account_base();

	inline crypto::secret_key_16 generate_new(uint8_t acc_opt)
	{
		crypto::generate_wallet_secret(m_keys.m_short_seed);
		m_acc_opt = acc_opt;
		m_creation_timestamp = time(NULL);
		return generate();
	}

	inline void recover(const crypto::secret_key_16 &recovery_seed, uint8_t acc_opt)
	{
		m_keys.m_short_seed = recovery_seed;
		m_acc_opt = acc_opt;
		m_creation_timestamp = EARLIEST_TIMESTAMP;
		generate();
	}

	void recover_legacy(const crypto::secret_key &recovery_seed);

	void create_from_device(const std::string &device_name);
	void create_from_keys(const cryptonote::account_public_address &address, const crypto::secret_key &spendkey, const crypto::secret_key &viewkey);
	void create_from_viewkey(const cryptonote::account_public_address &address, const crypto::secret_key &viewkey);
	bool make_multisig(const crypto::secret_key &view_secret_key, const crypto::secret_key &spend_secret_key, const crypto::public_key &spend_public_key, const std::vector<crypto::secret_key> &multisig_keys);
	void finalize_multisig(const crypto::public_key &spend_public_key);
	const account_keys &get_keys() const;
	std::string get_public_address_str(network_type nettype) const;
	std::string get_public_integrated_address_str(const crypto::hash8 &payment_id, network_type nettype) const;

	hw::device &get_device() const { return m_keys.get_device(); }
	void set_device(hw::device &hwdev) { m_keys.set_device(hwdev); }

	uint64_t get_createtime() const { return m_creation_timestamp; }
	void set_createtime(uint64_t val) { m_creation_timestamp = val; }

	uint8_t get_account_options() const { return m_acc_opt; }
	bool is_kurz() const { return m_keys.m_spend_secret_key == m_keys.m_view_secret_key; }

	bool has_25word_seed() const;
	bool has_14word_seed() const;

	bool load(const std::string &file_path);
	bool store(const std::string &file_path);

	void forget_spend_key();
	const std::vector<crypto::secret_key> &get_multisig_keys() const { return m_keys.m_multisig_keys; }

	template <class t_archive>
	inline void serialize(t_archive &a, const unsigned int /*ver*/)
	{
		a &m_keys;
		a &m_creation_timestamp;
	}

	static constexpr uint64_t EARLIEST_TIMESTAMP = 1483228800; // 01-01-2017 00:00

	BEGIN_KV_SERIALIZE_MAP()
	KV_SERIALIZE(m_keys)
	KV_SERIALIZE(m_creation_timestamp)
	KV_SERIALIZE(m_acc_opt)
	END_KV_SERIALIZE_MAP()

  private:
	crypto::secret_key_16 generate();
	void set_null();

	account_keys m_keys;
	uint64_t m_creation_timestamp;
	uint8_t m_acc_opt;
};
}

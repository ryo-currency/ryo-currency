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
// Parts of this file are originally copyright (c) 2012-2013 The Cryptonote developers

#include <fstream>

#include "account.h"
#include "crypto/crypto.h"
#include "include_base_utils.h"
#include "warnings.h"
extern "C" {
#include "crypto/keccak.h"
}
#include "cryptonote_basic_impl.h"
#include "cryptonote_format_utils.h"

//#undef RYO_DEFAULT_LOG_CATEGORY
//#define RYO_DEFAULT_LOG_CATEGORY "account"

using namespace std;

DISABLE_VS_WARNINGS(4244 4345)

namespace cryptonote
{

//-----------------------------------------------------------------
hw::device &account_keys::get_device() const
{
	return *m_device;
}
//-----------------------------------------------------------------
void account_keys::set_device(hw::device &hwdev)
{
	m_device = &hwdev;
	MCDEBUG("device", "account_keys::set_device device type: " << typeid(hwdev).name());
}

//-----------------------------------------------------------------
account_base::account_base()
{
	set_null();
}
//-----------------------------------------------------------------
void account_base::set_null()
{
	m_keys = account_keys();
}
//-----------------------------------------------------------------
void account_base::forget_spend_key()
{
	m_keys.m_spend_secret_key = crypto::secret_key();
	m_keys.m_multisig_keys.clear();
}
//-----------------------------------------------------------------
crypto::secret_key_16 account_base::generate()
{
	using namespace crypto;
	if(m_acc_opt == ACC_OPT_KURZ_ADDRESS)
	{
		crypto::generate_wallet_keys(m_keys.m_account_address.m_spend_public_key, m_keys.m_spend_secret_key, m_keys.m_short_seed, KEY_VARIANT_KURZ);
		m_keys.m_account_address.m_view_public_key = m_keys.m_account_address.m_spend_public_key;
		m_keys.m_view_secret_key = m_keys.m_spend_secret_key;
	}
	else
	{
		generate_wallet_keys(m_keys.m_account_address.m_spend_public_key, m_keys.m_spend_secret_key, m_keys.m_short_seed, KEY_VARIANT_SPENDKEY);
		generate_wallet_keys(m_keys.m_account_address.m_view_public_key, m_keys.m_view_secret_key, m_keys.m_short_seed, KEY_VARIANT_VIEWKEY);
	}

	return m_keys.m_short_seed;
}

void account_base::recover_legacy(const crypto::secret_key &recovery_seed)
{
	crypto::secret_key first = generate_legacy_keys(m_keys.m_account_address.m_spend_public_key, m_keys.m_spend_secret_key, recovery_seed, true);

	// rng for generating second set of keys is hash of first rng.  means only one set of electrum-style words needed for recovery
	crypto::secret_key second;
	keccak((uint8_t *)&m_keys.m_spend_secret_key, sizeof(crypto::secret_key), (uint8_t *)&second, sizeof(crypto::secret_key));

	generate_legacy_keys(m_keys.m_account_address.m_view_public_key, m_keys.m_view_secret_key, second, true);
	m_creation_timestamp = EARLIEST_TIMESTAMP;
}

bool account_base::has_25word_seed() const
{
	using namespace crypto;
	secret_key second;
	keccak((uint8_t *)&m_keys.m_spend_secret_key, sizeof(secret_key), (uint8_t *)&second, sizeof(secret_key));
	sc_reduce32((uint8_t *)unwrap(second).data);
	return second == m_keys.m_view_secret_key;
}

bool account_base::has_14word_seed() const
{
	using namespace crypto;
	if(m_acc_opt == ACC_OPT_UNKNOWN)
		return false;

	public_key pview, pspend;
	secret_key view, spend;
	if(m_acc_opt == ACC_OPT_KURZ_ADDRESS)
	{
		generate_wallet_keys(pspend, spend, m_keys.m_short_seed, KEY_VARIANT_KURZ);
		return spend == m_keys.m_spend_secret_key;
	}
	else
	{
		generate_wallet_keys(pspend, spend, m_keys.m_short_seed, KEY_VARIANT_SPENDKEY);
		generate_wallet_keys(pview, view, m_keys.m_short_seed, KEY_VARIANT_VIEWKEY);
		return spend == m_keys.m_spend_secret_key && view == m_keys.m_view_secret_key;
	}
}
//-----------------------------------------------------------------
void account_base::create_from_keys(const cryptonote::account_public_address &address, const crypto::secret_key &spendkey, const crypto::secret_key &viewkey)
{
	m_keys.m_account_address = address;
	m_keys.m_spend_secret_key = spendkey;
	m_keys.m_view_secret_key = viewkey;
	m_creation_timestamp = EARLIEST_TIMESTAMP;
}

//-----------------------------------------------------------------
void account_base::create_from_device(const std::string &device_name)
{

	hw::device &hwdev = hw::get_device(device_name);
	m_keys.set_device(hwdev);
	hwdev.set_name(device_name);
	MCDEBUG("ledger", "device type: " << typeid(hwdev).name());
	hwdev.init();
	hwdev.connect();
	hwdev.get_public_address(m_keys.m_account_address);
	hwdev.get_secret_keys(m_keys.m_view_secret_key, m_keys.m_spend_secret_key);
	m_creation_timestamp = EARLIEST_TIMESTAMP;
}

//-----------------------------------------------------------------
void account_base::create_from_viewkey(const cryptonote::account_public_address &address, const crypto::secret_key &viewkey)
{
	crypto::secret_key fake;
	fake.scrub();
	create_from_keys(address, fake, viewkey);
}
//-----------------------------------------------------------------
bool account_base::make_multisig(const crypto::secret_key &view_secret_key, const crypto::secret_key &spend_secret_key, const crypto::public_key &spend_public_key, const std::vector<crypto::secret_key> &multisig_keys)
{
	m_creation_timestamp = EARLIEST_TIMESTAMP;
	m_keys.m_account_address.m_spend_public_key = spend_public_key;
	m_keys.m_view_secret_key = view_secret_key;
	m_keys.m_spend_secret_key = spend_secret_key;
	m_keys.m_multisig_keys = multisig_keys;
	return crypto::secret_key_to_public_key(view_secret_key, m_keys.m_account_address.m_view_public_key);
}
//-----------------------------------------------------------------
void account_base::finalize_multisig(const crypto::public_key &spend_public_key)
{
	m_keys.m_account_address.m_spend_public_key = spend_public_key;
}
//-----------------------------------------------------------------
const account_keys &account_base::get_keys() const
{
	return m_keys;
}
//-----------------------------------------------------------------
std::string account_base::get_public_address_str(network_type nettype) const
{
	switch(nettype)
	{
	case MAINNET:
		return get_public_address_as_str<MAINNET>(false, m_keys.m_account_address);
	case TESTNET:
		return get_public_address_as_str<TESTNET>(false, m_keys.m_account_address);
	case STAGENET:
		return get_public_address_as_str<STAGENET>(false, m_keys.m_account_address);
	default:
		CHECK_AND_ASSERT_THROW_MES(false, "Unknown nettype");
	}
}
//-----------------------------------------------------------------
std::string account_base::get_public_integrated_address_str(const crypto::hash8 &payment_id, network_type nettype) const
{
	switch(nettype)
	{
	case MAINNET:
		return get_account_integrated_address_as_str<MAINNET>(m_keys.m_account_address, payment_id);
	case TESTNET:
		return get_account_integrated_address_as_str<TESTNET>(m_keys.m_account_address, payment_id);
	case STAGENET:
		return get_account_integrated_address_as_str<STAGENET>(m_keys.m_account_address, payment_id);
	default:
		CHECK_AND_ASSERT_THROW_MES(false, "Unknown nettype");
	}
}
//-----------------------------------------------------------------
}

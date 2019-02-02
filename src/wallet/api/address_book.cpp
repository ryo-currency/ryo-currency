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

#include "address_book.h"
#include "common_defines.h"
#include "crypto/hash.h"
#include "wallet.h"
#include "wallet/wallet2.h"

#include <vector>

namespace Ryo
{

AddressBook::~AddressBook() {}

AddressBookImpl::AddressBookImpl(WalletImpl *wallet)
	: m_wallet(wallet), m_errorCode(Status_Ok) {}

bool AddressBookImpl::addRow(const std::string &dst_addr, const std::string &payment_id_str, const std::string &description)
{
	clearStatus();

	cryptonote::address_parse_info info;
	if(!cryptonote::get_account_address_from_str(info, m_wallet->m_wallet->nettype(), dst_addr))
	{
		m_errorString = tr("Invalid destination address");
		m_errorCode = Invalid_Address;
		return false;
	}

	crypto::hash payment_id = crypto::null_hash;
	bool has_long_pid = (payment_id_str.empty()) ? false : tools::wallet2::parse_long_payment_id(payment_id_str, payment_id);

	// Short payment id provided
	if(payment_id_str.length() == 16)
	{
		m_errorString = tr("Invalid payment ID. Short payment ID should only be used in an integrated address");
		m_errorCode = Invalid_Payment_Id;
		return false;
	}

	// long payment id provided but not valid
	if(!payment_id_str.empty() && !has_long_pid)
	{
		m_errorString = tr("Invalid payment ID");
		m_errorCode = Invalid_Payment_Id;
		return false;
	}

	// integrated + long payment id provided
	if(has_long_pid && info.has_payment_id)
	{
		m_errorString = tr("Integrated address and long payment ID can't be used at the same time");
		m_errorCode = Invalid_Payment_Id;
		return false;
	}

	// Pad short pid with zeros
	if(info.has_payment_id)
	{
		memcpy(payment_id.data, info.payment_id.data, 8);
	}

	bool r = m_wallet->m_wallet->add_address_book_row(info.address, payment_id, description, info.is_subaddress);
	if(r)
		refresh();
	else
		m_errorCode = General_Error;
	return r;
}

void AddressBookImpl::refresh()
{
	LOG_PRINT_L2("Refreshing addressbook");

	clearRows();

	// Fetch from Wallet2 and create vector of AddressBookRow objects
	std::vector<tools::wallet2::address_book_row> rows = m_wallet->m_wallet->get_address_book();
	for(size_t i = 0; i < rows.size(); ++i)
	{
		tools::wallet2::address_book_row *row = &rows.at(i);

		std::string payment_id = (row->m_payment_id == crypto::null_hash) ? "" : epee::string_tools::pod_to_hex(row->m_payment_id);
		std::string address = cryptonote::get_account_address_as_str(m_wallet->m_wallet->nettype(), row->m_is_subaddress, row->m_address);
		// convert the zero padded short payment id to integrated address
		if(!row->m_is_subaddress && payment_id.length() > 16 && payment_id.substr(16).find_first_not_of('0') == std::string::npos)
		{
			payment_id = payment_id.substr(0, 16);
			crypto::hash8 payment_id_short;
			if(tools::wallet2::parse_short_payment_id(payment_id, payment_id_short))
			{
				address = cryptonote::get_account_integrated_address_as_str(m_wallet->m_wallet->nettype(), row->m_address, payment_id_short);
				// Don't show payment id when integrated address is used
				payment_id = "";
			}
		}
		AddressBookRow *abr = new AddressBookRow(i, address, payment_id, row->m_description);
		m_rows.push_back(abr);
	}
}

bool AddressBookImpl::deleteRow(std::size_t rowId)
{
	LOG_PRINT_L2("Deleting address book row " << rowId);
	bool r = m_wallet->m_wallet->delete_address_book_row(rowId);
	if(r)
		refresh();
	return r;
}

int AddressBookImpl::lookupPaymentID(const std::string &payment_id) const
{
	// turn short ones into long ones for comparison
	const std::string long_payment_id = payment_id + std::string(64 - payment_id.size(), '0');

	int idx = -1;
	for(const auto &row : m_rows)
	{
		++idx;
		// this does short/short and long/long
		if(payment_id == row->getPaymentId())
			return idx;
		// short/long
		if(long_payment_id == row->getPaymentId())
			return idx;
		// one case left: payment_id was long, row's is short
		const std::string long_row_payment_id = row->getPaymentId() + std::string(64 - row->getPaymentId().size(), '0');
		if(payment_id == long_row_payment_id)
			return idx;
	}
	return -1;
}

void AddressBookImpl::clearRows()
{
	for(auto r : m_rows)
	{
		delete r;
	}
	m_rows.clear();
}

void AddressBookImpl::clearStatus()
{
	m_errorString = "";
	m_errorCode = 0;
}

std::vector<AddressBookRow *> AddressBookImpl::getAll() const
{
	return m_rows;
}

AddressBookImpl::~AddressBookImpl()
{
	clearRows();
}

} // namespace

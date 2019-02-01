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

#include "subaddress.h"
#include "common_defines.h"
#include "crypto/hash.h"
#include "wallet.h"
#include "wallet/wallet2.h"

#include <vector>

namespace Ryo
{

Subaddress::~Subaddress() {}

SubaddressImpl::SubaddressImpl(WalletImpl *wallet)
	: m_wallet(wallet) {}

void SubaddressImpl::addRow(uint32_t accountIndex, const std::string &label)
{
	m_wallet->m_wallet->add_subaddress(accountIndex, label);
	refresh(accountIndex);
}

void SubaddressImpl::setLabel(uint32_t accountIndex, uint32_t addressIndex, const std::string &label)
{
	try
	{
		m_wallet->m_wallet->set_subaddress_label({accountIndex, addressIndex}, label);
		refresh(accountIndex);
	}
	catch(const std::exception &e)
	{
		LOG_ERROR("setLabel: " << e.what());
	}
}

void SubaddressImpl::refresh(uint32_t accountIndex)
{
	LOG_PRINT_L2("Refreshing subaddress");

	clearRows();
	for(size_t i = 0; i < m_wallet->m_wallet->get_num_subaddresses(accountIndex); ++i)
	{
		m_rows.push_back(new SubaddressRow(i, m_wallet->m_wallet->get_subaddress_as_str({accountIndex, (uint32_t)i}), m_wallet->m_wallet->get_subaddress_label({accountIndex, (uint32_t)i})));
	}
}

void SubaddressImpl::clearRows()
{
	for(auto r : m_rows)
	{
		delete r;
	}
	m_rows.clear();
}

std::vector<SubaddressRow *> SubaddressImpl::getAll() const
{
	return m_rows;
}

SubaddressImpl::~SubaddressImpl()
{
	clearRows();
}

} // namespace

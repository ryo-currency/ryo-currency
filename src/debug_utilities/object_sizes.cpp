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

#include "blockchain_db/lmdb/db_lmdb.h"
#include "cryptonote_basic/cryptonote_basic.h"
#include "cryptonote_basic/tx_extra.h"
#include "cryptonote_core/blockchain.h"
#include "cryptonote_protocol/cryptonote_protocol_handler.h"
#include "net/connection_basic.hpp"
#include "p2p/net_node.h"
#include "p2p/net_peerlist.h"
#include "p2p/p2p_protocol_defs.h"
#include "wallet/api/pending_transaction.h"
#include "wallet/api/transaction_history.h"
#include "wallet/api/transaction_info.h"
#include "wallet/api/unsigned_transaction.h"
#include "wallet/api/wallet.h"
#include "wallet/wallet2.h"
#include <map>

//#undef RYO_DEFAULT_LOG_CATEGORY
//#define RYO_DEFAULT_LOG_CATEGORY "debugtools.objectsizes"

class size_logger
{
  public:
	~size_logger()
	{
		for(const auto &i : types)
			std::cout << std::to_string(i.first) << "\t" << i.second << std::endl;
	}
	void add(const char *type, size_t size) { types.insert(std::make_pair(size, type)); }
  private:
	std::multimap<size_t, const std::string> types;
};
#define SL(type) sl.add(#type, sizeof(type))

int main(int argc, char *argv[])
{
	size_logger sl;

	tools::on_startup();

	mlog_configure("", true);

	SL(boost::thread);
	SL(boost::asio::io_service);
	SL(boost::asio::io_service::work);
	SL(boost::asio::deadline_timer);

	SL(cryptonote::DB_ERROR);
	SL(cryptonote::mdb_txn_safe);
	SL(cryptonote::mdb_threadinfo);

	SL(cryptonote::block_header);
	SL(cryptonote::block);
	SL(cryptonote::transaction_prefix);
	SL(cryptonote::transaction);

	SL(cryptonote::txpool_tx_meta_t);

	SL(epee::net_utils::ipv4_network_address);
	SL(epee::net_utils::network_address);
	SL(epee::net_utils::connection_context_base);
	SL(epee::net_utils::connection_basic);

	SL(nodetool::peerlist_entry);
	SL(nodetool::anchor_peerlist_entry);
	SL(nodetool::node_server<cryptonote::t_cryptonote_protocol_handler<cryptonote::core>>);
	SL(nodetool::p2p_connection_context_t<cryptonote::t_cryptonote_protocol_handler<cryptonote::core>::connection_context>);
	SL(nodetool::network_address_old);

	SL(tools::wallet2::transfer_details);
	SL(tools::wallet2::payment_details);
	SL(tools::wallet2::unconfirmed_transfer_details);
	SL(tools::wallet2::confirmed_transfer_details);
	SL(tools::wallet2::tx_construction_data);
	SL(tools::wallet2::pending_tx);
	SL(tools::wallet2::unsigned_tx_set);
	SL(tools::wallet2::signed_tx_set);

	SL(Ryo::WalletImpl);
	SL(Ryo::AddressBookRow);
	SL(Ryo::TransactionInfoImpl);
	SL(Ryo::TransactionHistoryImpl);
	SL(Ryo::PendingTransactionImpl);
	SL(Ryo::UnsignedTransactionImpl);

	return 0;
}

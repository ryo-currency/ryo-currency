/**
@file
@details

@image html images/other/runtime-commands.png

*/

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

#pragma once

#include <boost/optional/optional.hpp>

#include "common/common_fwd.h"
#include "daemon/rpc_command_executor.h"
#include "rpc/core_rpc_server.h"

namespace daemonize
{

class t_command_parser_executor final
{
  private:
	t_rpc_command_executor m_executor;

  public:
	t_command_parser_executor(
		uint32_t ip, uint16_t port, const boost::optional<tools::login> &login, bool is_rpc, cryptonote::core_rpc_server *rpc_server = NULL);

	bool print_peer_list(const std::vector<std::string> &args);

	bool print_peer_list_stats(const std::vector<std::string> &args);

	bool save_blockchain(const std::vector<std::string> &args);

	bool show_hash_rate(const std::vector<std::string> &args);

	bool hide_hash_rate(const std::vector<std::string> &args);

	bool show_difficulty(const std::vector<std::string> &args);

	bool show_status(const std::vector<std::string> &args);

	bool print_connections(const std::vector<std::string> &args);

	bool print_blockchain_info(const std::vector<std::string> &args);

	bool set_log_level(const std::vector<std::string> &args);

	bool set_log_categories(const std::vector<std::string> &args);

	bool print_height(const std::vector<std::string> &args);

	bool print_block(const std::vector<std::string> &args);

	bool print_transaction(const std::vector<std::string> &args);

	bool is_key_image_spent(const std::vector<std::string> &args);

	bool print_transaction_pool_long(const std::vector<std::string> &args);

	bool print_transaction_pool_short(const std::vector<std::string> &args);

	bool print_transaction_pool_stats(const std::vector<std::string> &args);

	bool start_mining(const std::vector<std::string> &args);

	bool stop_mining(const std::vector<std::string> &args);

	bool stop_daemon(const std::vector<std::string> &args);

	bool print_status(const std::vector<std::string> &args);

	bool set_limit(const std::vector<std::string> &args);

	bool set_limit_up(const std::vector<std::string> &args);

	bool set_limit_down(const std::vector<std::string> &args);

	bool out_peers(const std::vector<std::string> &args);

	bool in_peers(const std::vector<std::string> &args);

	bool hard_fork_info(const std::vector<std::string> &args);

	bool show_bans(const std::vector<std::string> &args);

	bool ban(const std::vector<std::string> &args);

	bool unban(const std::vector<std::string> &args);

	bool flush_txpool(const std::vector<std::string> &args);

	bool output_histogram(const std::vector<std::string> &args);

	bool print_coinbase_tx_sum(const std::vector<std::string> &args);

	bool alt_chain_info(const std::vector<std::string> &args);

	bool print_blockchain_dynamic_stats(const std::vector<std::string> &args);

	bool update(const std::vector<std::string> &args);

	bool relay_tx(const std::vector<std::string> &args);

	bool sync_info(const std::vector<std::string> &args);

	bool version(const std::vector<std::string> &args);
};

} // namespace daemonize

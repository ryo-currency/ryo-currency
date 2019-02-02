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

#ifndef DAEMON_COMMAND_LINE_ARGS_H
#define DAEMON_COMMAND_LINE_ARGS_H

#include "common/command_line.h"
#include "cryptonote_config.h"
#include "daemonizer/daemonizer.h"

namespace daemon_args
{
std::string const WINDOWS_SERVICE_NAME = "Ryo Daemon";

const command_line::arg_descriptor<std::string, false, true, 2> arg_config_file = {
	"config-file", "Specify configuration file", (daemonizer::get_default_data_dir() / std::string(CRYPTONOTE_NAME ".conf")).string(), {{&cryptonote::arg_testnet_on, &cryptonote::arg_stagenet_on}}, [](std::array<bool, 2> testnet_stagenet, bool defaulted, std::string val) -> std::string {
      if (testnet_stagenet[0] && defaulted)
        return (daemonizer::get_default_data_dir() / "testnet" /
                std::string(CRYPTONOTE_NAME ".conf")).string();
      else if (testnet_stagenet[1] && defaulted)
        return (daemonizer::get_default_data_dir() / "stagenet" /
                std::string(CRYPTONOTE_NAME ".conf")).string();
      return val; }};
const command_line::arg_descriptor<std::string, false, true, 2> arg_log_file = {
	"log-file", "Specify log file", (daemonizer::get_default_data_dir() / std::string(CRYPTONOTE_NAME ".log")).string(), {{&cryptonote::arg_testnet_on, &cryptonote::arg_stagenet_on}}, [](std::array<bool, 2> testnet_stagenet, bool defaulted, std::string val) -> std::string {
      if (testnet_stagenet[0] && defaulted)
        return (daemonizer::get_default_data_dir() / "testnet" /
                std::string(CRYPTONOTE_NAME ".log")).string();
      else if (testnet_stagenet[1] && defaulted)
        return (daemonizer::get_default_data_dir() / "stagenet" /
                std::string(CRYPTONOTE_NAME ".log")).string();
      return val; }};
const command_line::arg_descriptor<std::size_t> arg_max_log_file_size = {
	"max-log-file-size", "Specify maximum log file size [B]", MAX_LOG_FILE_SIZE};
const command_line::arg_descriptor<std::string> arg_log_level = {
	"log-level", "", ""};
const command_line::arg_descriptor<std::vector<std::string>> arg_command = {
	"daemon_command", "Hidden"};
const command_line::arg_descriptor<bool> arg_os_version = {
	"os-version", "OS for which this executable was compiled"};
const command_line::arg_descriptor<unsigned> arg_max_concurrency = {
	"max-concurrency", "Max number of threads to use for a parallel job", 0};

const command_line::arg_descriptor<std::string> arg_zmq_rpc_bind_ip = {
	"zmq-rpc-bind-ip", "IP for ZMQ RPC server to listen on", "127.0.0.1"};

const command_line::arg_descriptor<std::string, false, true, 2> arg_zmq_rpc_bind_port = {
	"zmq-rpc-bind-port", "Port for ZMQ RPC server to listen on", std::to_string(cryptonote::config<cryptonote::MAINNET>::ZMQ_RPC_DEFAULT_PORT), {{&cryptonote::arg_testnet_on, &cryptonote::arg_stagenet_on}}, [](std::array<bool, 2> testnet_stagenet, bool defaulted, std::string val) -> std::string {
      if (testnet_stagenet[0] && defaulted)
        return std::to_string(cryptonote::config<cryptonote::TESTNET>::ZMQ_RPC_DEFAULT_PORT);
      if (testnet_stagenet[1] && defaulted)
        return std::to_string(cryptonote::config<cryptonote::STAGENET>::ZMQ_RPC_DEFAULT_PORT);
      return val; }};

} // namespace daemon_args

#endif // DAEMON_COMMAND_LINE_ARGS_H

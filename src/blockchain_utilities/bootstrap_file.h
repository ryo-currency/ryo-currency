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

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/iostreams/device/back_inserter.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/stream_buffer.hpp>

#include "cryptonote_basic/cryptonote_basic.h"
#include "cryptonote_core/blockchain.h"

#include <algorithm>
#include <atomic>
#include <boost/iostreams/copy.hpp>
#include <cstdio>
#include <fstream>

#include "common/command_line.h"
#include "version.h"

#include "blockchain_utilities.h"

using namespace cryptonote;

class BootstrapFile
{
  public:
	uint64_t count_bytes(std::ifstream &import_file, uint64_t blocks, uint64_t &h, bool &quit);
	uint64_t count_blocks(const std::string &dir_path, std::streampos &start_pos, uint64_t &seek_height);
	uint64_t count_blocks(const std::string &dir_path);
	uint64_t seek_to_first_chunk(std::ifstream &import_file);

	bool store_blockchain_raw(cryptonote::Blockchain *cs, cryptonote::tx_memory_pool *txp,
							  boost::filesystem::path &output_file, uint64_t use_block_height = 0);

  protected:
	Blockchain *m_blockchain_storage;

	tx_memory_pool *m_tx_pool;
	typedef std::vector<char> buffer_type;
	std::ofstream *m_raw_data_file;
	buffer_type m_buffer;
	boost::iostreams::stream<boost::iostreams::back_insert_device<buffer_type>> *m_output_stream;

	// open export file for write
	bool open_writer(const boost::filesystem::path &file_path);
	bool initialize_file();
	bool close();
	void write_block(block &block);
	void flush_chunk();

  private:
	uint64_t m_height;
	uint64_t m_cur_height; // tracks current height during export
	uint32_t m_max_chunk;
};

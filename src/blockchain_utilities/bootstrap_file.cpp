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

#include "bootstrap_serialization.h"
#include "serialization/binary_utils.h" // dump_binary(), parse_binary()
#include "serialization/json_utils.h" // dump_json()

#include "bootstrap_file.h"

#include "common/gulps.hpp"

GULPS_CAT_MAJOR("bootstr_file");

namespace po = boost::program_options;

using namespace cryptonote;
using namespace epee;

namespace
{
// This number was picked by taking the leading 4 bytes from this output:
// echo Monero bootstrap file | sha1sum
const uint32_t blockchain_raw_magic = 0x28721586;
const uint32_t header_size = 1024;

std::string refresh_string = "\r                                    \r";
} // namespace

bool BootstrapFile::open_writer(const boost::filesystem::path &file_path)
{
	const boost::filesystem::path dir_path = file_path.parent_path();
	if(!dir_path.empty())
	{
		if(boost::filesystem::exists(dir_path))
		{
			if(!boost::filesystem::is_directory(dir_path))
			{
				GULPS_ERROR("export directory path is a file: ", dir_path);
				return false;
			}
		}
		else
		{
			if(!boost::filesystem::create_directory(dir_path))
			{
				GULPS_ERROR("Failed to create directory ", dir_path);
				return false;
			}
		}
	}

	m_raw_data_file = new std::ofstream();

	bool do_initialize_file = false;
	uint64_t num_blocks = 0;

	if(!boost::filesystem::exists(file_path))
	{
		GULPS_LOG_L1("creating file");
		do_initialize_file = true;
		num_blocks = 0;
	}
	else
	{
		num_blocks = count_blocks(file_path.string());
		GULPSF_LOG_L1("appending to existing file with height: {}  total blocks: {}", num_blocks - 1, num_blocks);
	}
	m_height = num_blocks;

	if(do_initialize_file)
		m_raw_data_file->open(file_path.string(), std::ios_base::binary | std::ios_base::out | std::ios::trunc);
	else
		m_raw_data_file->open(file_path.string(), std::ios_base::binary | std::ios_base::out | std::ios::app | std::ios::ate);

	if(m_raw_data_file->fail())
		return false;

	m_output_stream = new boost::iostreams::stream<boost::iostreams::back_insert_device<buffer_type>>(m_buffer);
	if(m_output_stream == nullptr)
		return false;

	if(do_initialize_file)
		initialize_file();

	return true;
}

bool BootstrapFile::initialize_file()
{
	const uint32_t file_magic = blockchain_raw_magic;

	std::string blob;
	if(!::serialization::dump_binary(file_magic, blob))
	{
		throw std::runtime_error("Error in serialization of file magic");
	}
	*m_raw_data_file << blob;

	bootstrap::file_info bfi;
	bfi.major_version = 0;
	bfi.minor_version = 1;
	bfi.header_size = header_size;

	bootstrap::blocks_info bbi;
	bbi.block_first = 0;
	bbi.block_last = 0;
	bbi.block_last_pos = 0;

	buffer_type buffer2;
	boost::iostreams::stream<boost::iostreams::back_insert_device<buffer_type>> output_stream_header(buffer2);

	uint32_t bd_size = 0;

	blobdata bd = t_serializable_object_to_blob(bfi);
	GULPSF_LOG_L1("bootstrap::file_info size: {}", bd.size());
	bd_size = bd.size();

	if(!::serialization::dump_binary(bd_size, blob))
	{
		throw std::runtime_error("Error in serialization of bootstrap::file_info size");
	}
	output_stream_header << blob;
	output_stream_header << bd;

	bd = t_serializable_object_to_blob(bbi);
	GULPSF_LOG_L1("bootstrap::blocks_info size: {}", bd.size());
	bd_size = bd.size();

	if(!::serialization::dump_binary(bd_size, blob))
	{
		throw std::runtime_error("Error in serialization of bootstrap::blocks_info size");
	}
	output_stream_header << blob;
	output_stream_header << bd;

	output_stream_header.flush();
	output_stream_header << std::string(header_size - buffer2.size(), 0); // fill in rest with null bytes
	output_stream_header.flush();
	std::copy(buffer2.begin(), buffer2.end(), std::ostreambuf_iterator<char>(*m_raw_data_file));

	return true;
}

void BootstrapFile::flush_chunk()
{
	m_output_stream->flush();

	uint32_t chunk_size = m_buffer.size();
	// MTRACE("chunk_size " << chunk_size);
	if(chunk_size > BUFFER_SIZE)
	{
		GULPSF_WARN("WARNING: chunk_size {} > BUFFER_SIZE {}", chunk_size, BUFFER_SIZE);
	}

	std::string blob;
	if(!::serialization::dump_binary(chunk_size, blob))
	{
		throw std::runtime_error("Error in serialization of chunk size");
	}
	*m_raw_data_file << blob;

	if(m_max_chunk < chunk_size)
	{
		m_max_chunk = chunk_size;
	}
	long pos_before = m_raw_data_file->tellp();
	std::copy(m_buffer.begin(), m_buffer.end(), std::ostreambuf_iterator<char>(*m_raw_data_file));
	m_raw_data_file->flush();
	long pos_after = m_raw_data_file->tellp();
	long num_chars_written = pos_after - pos_before;
	if(static_cast<unsigned long>(num_chars_written) != chunk_size)
	{
		GULPSF_ERROR("Error writing chunk:  height: {}  chunk_size: {}  num chars written: {}", m_cur_height, chunk_size, num_chars_written);
		throw std::runtime_error("Error writing chunk");
	}

	m_buffer.clear();
	delete m_output_stream;
	m_output_stream = new boost::iostreams::stream<boost::iostreams::back_insert_device<buffer_type>>(m_buffer);
	GULPSF_LOG_L1("flushed chunk:  chunk_size: {}", chunk_size);
}

void BootstrapFile::write_block(block &block)
{
	bootstrap::block_package bp;
	bp.block = block;

	std::vector<transaction> txs;

	uint64_t block_height = boost::get<txin_gen>(block.miner_tx.vin.front()).height;

	// now add all regular transactions
	for(const auto &tx_id : block.tx_hashes)
	{
		if(tx_id == crypto::null_hash)
		{
			throw std::runtime_error("Aborting: tx == null_hash");
		}
		transaction tx = m_blockchain_storage->get_db().get_tx(tx_id);

		txs.push_back(tx);
	}

	// these non-coinbase txs will be serialized using this structure
	bp.txs = txs;

	// These three attributes are currently necessary for a fast import that adds blocks without verification.
	bool include_extra_block_data = true;
	if(include_extra_block_data)
	{
		size_t block_size = m_blockchain_storage->get_db().get_block_size(block_height);
		difficulty_type cumulative_difficulty = m_blockchain_storage->get_db().get_block_cumulative_difficulty(block_height);
		uint64_t coins_generated = m_blockchain_storage->get_db().get_block_already_generated_coins(block_height);

		bp.block_size = block_size;
		bp.cumulative_difficulty = cumulative_difficulty;
		bp.coins_generated = coins_generated;
	}

	blobdata bd = t_serializable_object_to_blob(bp);
	m_output_stream->write((const char *)bd.data(), bd.size());
}

bool BootstrapFile::close()
{
	if(m_raw_data_file->fail())
		return false;

	m_raw_data_file->flush();
	delete m_output_stream;
	delete m_raw_data_file;
	return true;
}

bool BootstrapFile::store_blockchain_raw(Blockchain *_blockchain_storage, tx_memory_pool *_tx_pool, boost::filesystem::path &output_file, uint64_t requested_block_stop)
{
	uint64_t num_blocks_written = 0;
	m_max_chunk = 0;
	m_blockchain_storage = _blockchain_storage;
	m_tx_pool = _tx_pool;
	uint64_t progress_interval = 100;
	GULPS_INFO("Storing blocks raw data...");
	if(!BootstrapFile::open_writer(output_file))
	{
		GULPS_ERROR("failed to open raw file for write");
		return false;
	}
	block b;

	// block_start, block_stop use 0-based height. m_height uses 1-based height. So to resume export
	// from last exported block, block_start doesn't need to add 1 here, as it's already at the next
	// height.
	uint64_t block_start = m_height;
	uint64_t block_stop = 0;
	GULPSF_INFO("source blockchain height: {}", m_blockchain_storage->get_current_blockchain_height() - 1);
	if((requested_block_stop > 0) && (requested_block_stop < m_blockchain_storage->get_current_blockchain_height()))
	{
		GULPSF_INFO("Using requested block height: {}", requested_block_stop);
		block_stop = requested_block_stop;
	}
	else
	{
		block_stop = m_blockchain_storage->get_current_blockchain_height() - 1;
		GULPSF_INFO("Using block height of source blockchain: {}", block_stop);
	}
	for(m_cur_height = block_start; m_cur_height <= block_stop; ++m_cur_height)
	{
		// this method's height refers to 0-based height (genesis block = height 0)
		crypto::hash hash = m_blockchain_storage->get_block_id_by_height(m_cur_height);
		m_blockchain_storage->get_block_by_hash(hash, b);
		write_block(b);
		if(m_cur_height % NUM_BLOCKS_PER_CHUNK == 0)
		{
			flush_chunk();
			num_blocks_written += NUM_BLOCKS_PER_CHUNK;
		}
		if(m_cur_height % progress_interval == 0)
		{
			GULPS_PRINT(refresh_string);
			GULPSF_PRINT("block {}/{}\r", m_cur_height, block_stop);
		}
	}
	// NOTE: use of NUM_BLOCKS_PER_CHUNK is a placeholder in case multi-block chunks are later supported.
	if(m_cur_height % NUM_BLOCKS_PER_CHUNK != 0)
	{
		flush_chunk();
	}
	// print message for last block, which may not have been printed yet due to progress_interval
	GULPS_PRINT(refresh_string);
	GULPSF_PRINT("block {}/{}", m_cur_height - 1, block_stop);

	GULPSF_INFO("Number of blocks exported: {}", num_blocks_written);
	if(num_blocks_written > 0)
		GULPSF_INFO("Largest chunk: {} bytes", m_max_chunk);

	return BootstrapFile::close();
}

uint64_t BootstrapFile::seek_to_first_chunk(std::ifstream &import_file)
{
	uint32_t file_magic;

	std::string str1;
	char buf1[2048];
	import_file.read(buf1, sizeof(file_magic));
	if(!import_file)
		throw std::runtime_error("Error reading expected number of bytes");
	str1.assign(buf1, sizeof(file_magic));

	if(!::serialization::parse_binary(str1, file_magic))
		throw std::runtime_error("Error in deserialization of file_magic");

	if(file_magic != blockchain_raw_magic)
	{
		GULPS_ERROR("bootstrap file not recognized");
		throw std::runtime_error("Aborting");
	}
	else
		GULPS_INFO("bootstrap file recognized");

	uint32_t buflen_file_info;

	import_file.read(buf1, sizeof(buflen_file_info));
	str1.assign(buf1, sizeof(buflen_file_info));
	if(!import_file)
		throw std::runtime_error("Error reading expected number of bytes");
	if(!::serialization::parse_binary(str1, buflen_file_info))
		throw std::runtime_error("Error in deserialization of buflen_file_info");
	GULPSF_INFO("bootstrap::file_info size: {}", buflen_file_info);

	if(buflen_file_info > sizeof(buf1))
		throw std::runtime_error("Error: bootstrap::file_info size exceeds buffer size");
	import_file.read(buf1, buflen_file_info);
	if(!import_file)
		throw std::runtime_error("Error reading expected number of bytes");
	str1.assign(buf1, buflen_file_info);
	bootstrap::file_info bfi;
	if(!::serialization::parse_binary(str1, bfi))
		throw std::runtime_error("Error in deserialization of bootstrap::file_info");
	GULPSF_INFO("bootstrap file v {}.{}", unsigned(bfi.major_version), unsigned(bfi.minor_version));
	GULPSF_INFO("bootstrap magic size: {}", sizeof(file_magic));
	GULPSF_INFO("bootstrap header size: {}", bfi.header_size);

	uint64_t full_header_size = sizeof(file_magic) + bfi.header_size;
	import_file.seekg(full_header_size);

	return full_header_size;
}

uint64_t BootstrapFile::count_bytes(std::ifstream &import_file, uint64_t blocks, uint64_t &h, bool &quit)
{
	uint64_t bytes_read = 0;
	uint32_t chunk_size;
	char buf1[sizeof(chunk_size)];
	std::string str1;
	h = 0;
	while(1)
	{
		import_file.read(buf1, sizeof(chunk_size));
		if(!import_file)
		{
			GULPS_PRINT(refresh_string);
			GULPS_LOG_L1("End of file reached");
			quit = true;
			break;
		}
		bytes_read += sizeof(chunk_size);
		str1.assign(buf1, sizeof(chunk_size));
		if(!::serialization::parse_binary(str1, chunk_size))
			throw std::runtime_error("Error in deserialization of chunk_size");
		GULPSF_LOG_L1("chunk_size: {}", chunk_size);

		if(chunk_size > BUFFER_SIZE)
		{
			GULPS_PRINT(refresh_string);
			GULPSF_WARN("WARNING: chunk_size {} > BUFFER_SIZE {}  height: {}", chunk_size, BUFFER_SIZE, h - 1);
			throw std::runtime_error("Aborting: chunk size exceeds buffer size");
		}
		if(chunk_size > CHUNK_SIZE_WARNING_THRESHOLD)
		{
			GULPS_PRINT(refresh_string);
			GULPSF_LOG_L1("NOTE: chunk_size {} > {} << height: {}", chunk_size, CHUNK_SIZE_WARNING_THRESHOLD, h - 1);
		}
		else if(chunk_size <= 0)
		{
			GULPS_PRINT(refresh_string);
			GULPSF_LOG_L1("ERROR: chunk_size {} <= 0  height: {}", chunk_size, h - 1);
			throw std::runtime_error("Aborting");
		}
		// skip to next expected block size value
		import_file.seekg(chunk_size, std::ios_base::cur);
		if(!import_file)
		{
			GULPS_PRINT(refresh_string);
			GULPSF_ERROR("ERROR: unexpected end of file: bytes read before error: {} of chunk_size {}", import_file.gcount(), chunk_size);
			throw std::runtime_error("Aborting");
		}
		bytes_read += chunk_size;
		h += NUM_BLOCKS_PER_CHUNK;
		if(h >= blocks)
			break;
	}
	return bytes_read;
}

uint64_t BootstrapFile::count_blocks(const std::string &import_file_path)
{
	std::streampos dummy_pos;
	uint64_t dummy_height = 0;
	return count_blocks(import_file_path, dummy_pos, dummy_height);
}

// If seek_height is non-zero on entry, return a stream position <= this height when finished.
// And return the actual height corresponding to this position. Allows the caller to locate its
// starting position without having to reread the entire file again.
uint64_t BootstrapFile::count_blocks(const std::string &import_file_path, std::streampos &start_pos, uint64_t &seek_height)
{
	boost::filesystem::path raw_file_path(import_file_path);
	boost::system::error_code ec;
	if(!boost::filesystem::exists(raw_file_path, ec))
	{
		GULPS_ERROR("bootstrap file not found: ", raw_file_path);
		throw std::runtime_error("Aborting");
	}
	std::ifstream import_file;
	import_file.open(import_file_path, std::ios_base::binary | std::ifstream::in);

	uint64_t start_height = seek_height;
	uint64_t h = 0;
	if(import_file.fail())
	{
		GULPS_ERROR("import_file.open() fail");
		throw std::runtime_error("Aborting");
	}

	uint64_t full_header_size; // 4 byte magic + length of header structures
	full_header_size = seek_to_first_chunk(import_file);

	GULPS_INFO("Scanning blockchain from bootstrap file...");
	bool quit = false;
	uint64_t bytes_read = 0, blocks;
	int progress_interval = 10;

	while(!quit)
	{
		if(start_height && h + progress_interval >= start_height - 1)
		{
			start_height = 0;
			start_pos = import_file.tellg();
			seek_height = h;
		}
		bytes_read += count_bytes(import_file, progress_interval, blocks, quit);
		h += blocks;
		GULPSF_PRINT("\rblock height: {}    \r", h - 1);

		// GULPS_PRINT( refresh_string);
		GULPSF_LOG_L1("Number bytes scanned: {}", bytes_read);
	}

	import_file.close();

	GULPS_PRINT("\n");
	GULPS_PRINT("Done scanning bootstrap file");
	GULPSF_PRINT("Full header length: {} bytes", full_header_size);
	GULPSF_PRINT("Scanned for blocks: {} bytes", bytes_read);
	GULPSF_PRINT("Total:              {} bytes", full_header_size + bytes_read);
	GULPSF_PRINT("Number of blocks: {}", h);
	GULPS_PRINT("\n");

	// NOTE: h is the number of blocks.
	// Note that a block's stored height is zero-based, but parts of the code use
	// one-based height.
	return h;
}

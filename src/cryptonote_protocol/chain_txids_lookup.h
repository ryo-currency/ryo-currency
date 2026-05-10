// Copyright (c) 2025
#pragma once

#include <boost/optional.hpp>
#include <cstdint>
#include <string>

namespace cryptonote
{

// Minimal protocol-facing view of the optional chain_txids database.
// The concrete SQLite helper lives in daemon/, keeping protocol code independent
// from the daemon executable wrapper.
struct ChainTxidEntry
{
	// Height where the txid is expected to appear in the local chain snapshot.
	uint64_t height;

	// Hex block hash from the snapshot, used to verify the tx belongs to this block.
	std::string block_hash;
};

class i_chain_txids_lookup
{
  public:
	virtual ~i_chain_txids_lookup() = default;

	// Highest block height covered by the snapshot; newer peer blocks are skipped.
	virtual boost::optional<uint64_t> get_max_height() const = 0;

	// Lookup by canonical hex txid and return the snapshot location if present.
	virtual boost::optional<ChainTxidEntry> lookup_txid(const std::string &txid) const = 0;
};

} // namespace cryptonote

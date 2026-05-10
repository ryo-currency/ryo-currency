// Copyright (c) 2025
#pragma once

#include <string>
#include <boost/optional.hpp>
#include <sqlite3.h>

#include "cryptonote_protocol/chain_txids_lookup.h"

namespace daemonize
{

using ChainTxidEntry = cryptonote::ChainTxidEntry;

// SQLite helper for reading on-chain transaction metadata from the chain_txids database
// Provides read-only access to pre-computed transaction metadata for fast lookups
// Thread-safe: relies on SQLite's SQLITE_THREADSAFE=1 (serialized mode) which provides
// internal mutex protection for all connection access, allowing safe concurrent reads.
class ChainTxidsDb : public cryptonote::i_chain_txids_lookup
{
public:
  // Initialize the helper with the path to the chain_txids SQLite database
  explicit ChainTxidsDb(const std::string &db_path);
  ~ChainTxidsDb();

  // Prevent copying to avoid double-closing the database
  ChainTxidsDb(const ChainTxidsDb &) = delete;
  ChainTxidsDb &operator=(const ChainTxidsDb &) = delete;

  // Open the database in read-only mode
  // Returns true on success, false on error
  bool init();

  // Query the maximum height in the database using SELECT MAX(height)
  // Returns the max height if successful, or boost::none if not found/error
  boost::optional<uint64_t> get_max_height() const override;

  // Look up a transaction by ID in the database
  // Returns height and block_hash for the given txid, or boost::none if not found
  boost::optional<ChainTxidEntry> lookup_txid(const std::string &txid) const override;


  // Run self-tests to validate database connectivity and functionality
  // Tests: known txid lookup (should HIT), fake txid lookup (should MISS), max height query
  void run_self_tests();

private:
  std::string db_path_; // Path to the SQLite database file
  sqlite3 *db_;         // SQLite database connection handle
};

} // namespace daemonize

// Copyright (c) 2025
#include "daemon/chain_txids_db.h"
#include "common/gulps.hpp"
#include <boost/optional.hpp>
#include <iostream>

GULPS_CAT_MAJOR("chain_txids");

namespace daemonize
{

// Constructor: store the database path for later initialization
ChainTxidsDb::ChainTxidsDb(const std::string &db_path)
    : db_path_(db_path), db_(nullptr)
{
}

// Destructor: close the database connection if it was opened
ChainTxidsDb::~ChainTxidsDb()
{
  if(db_)
  {
    sqlite3_close(db_);
    db_ = nullptr;
  }
}

// Open the chain_txids database in read-only mode
// Returns true if successful, false if the database cannot be opened
bool ChainTxidsDb::init()
{
  // Open the database with read-only flag to prevent accidental modifications
  int rc = sqlite3_open_v2(
      db_path_.c_str(),
      &db_,
      SQLITE_OPEN_READONLY,
      nullptr);

  if(rc != SQLITE_OK)
  {
    // Log error to both gulps and stderr for visibility
    GULPSF_ERROR("Failed to open chain_txids database at {}: {}", db_path_, sqlite3_errmsg(db_));
    std::cerr << "[chain_txids] ERROR: Failed to open database at " << db_path_ << std::endl;
    return false;
  }

  // Log successful initialization
  GULPSF_INFO("Opened chain_txids database at {}", db_path_);
  std::cerr << "[chain_txids] INFO: Opened database at " << db_path_ << std::endl;
  return true;
}

// Query the maximum block height in the database
// Returns the max height value, or boost::none if not found or on error
boost::optional<uint64_t> ChainTxidsDb::get_max_height() const
{
  // Return none if database hasn't been initialized
  if(!db_)
    return boost::none;

  sqlite3_stmt *stmt = nullptr;
  const char *sql = "SELECT MAX(height) FROM chain_txids;";

  // Prepare the SQL statement
  int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
  if(rc != SQLITE_OK)
  {
    GULPSF_ERROR("Failed to prepare statement: {}", sqlite3_errmsg(db_));
    return boost::none;
  }

  // Execute the query and extract the result
  boost::optional<uint64_t> result;
  if(sqlite3_step(stmt) == SQLITE_ROW)
  {
    result = static_cast<uint64_t>(sqlite3_column_int64(stmt, 0));
  }

  // Clean up the prepared statement
  sqlite3_finalize(stmt);
  return result;
}

// Look up a transaction ID in the database
// Returns height and block_hash for the given txid, or boost::none if not found
boost::optional<ChainTxidEntry> ChainTxidsDb::lookup_txid(const std::string &txid) const
{
  // Return none if database hasn't been initialized
  if(!db_)
    return boost::none;

  sqlite3_stmt *stmt = nullptr;
  const char *sql = "SELECT height, block_hash FROM chain_txids WHERE txid = ? LIMIT 1;";

  // Prepare the parameterized SQL statement (prevents SQL injection)
  int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
  if(rc != SQLITE_OK)
  {
    GULPSF_ERROR("Failed to prepare statement: {}", sqlite3_errmsg(db_));
    return boost::none;
  }

  // Bind the txid parameter to the prepared statement
  sqlite3_bind_text(stmt, 1, txid.c_str(), -1, SQLITE_STATIC);

  // Execute the query and extract results
  boost::optional<ChainTxidEntry> result;
  if(sqlite3_step(stmt) == SQLITE_ROW)
  {
    ChainTxidEntry entry;
    entry.height = static_cast<uint64_t>(sqlite3_column_int64(stmt, 0));
    const unsigned char *hash_ptr = sqlite3_column_text(stmt, 1);
    if(hash_ptr)
    {
      entry.block_hash = reinterpret_cast<const char *>(hash_ptr);
    }
    result = entry;
  }

  // Clean up the prepared statement
  sqlite3_finalize(stmt);
  return result;
}

// Run validation tests on the database during daemon startup
// Tests database connectivity and confirms data is accessible
void ChainTxidsDb::run_self_tests()
{
  GULPS_INFO("Running chain_txids database self-tests...");
  std::cerr << "[chain_txids] Running self-tests..." << std::endl;

  // TEST 1: Dynamically fetch a real transaction and verify it can be found with correct height
  if(!db_)
  {
    GULPS_ERROR("Self-test FAIL: database not initialized");
    std::cerr << "[chain_txids] Self-test FAIL: database not initialized" << std::endl;
    return;
  }

  sqlite3_stmt *stmt = nullptr;
  const char *sql = "SELECT txid, height FROM chain_txids LIMIT 1;";
  int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);

  if(rc != SQLITE_OK)
  {
    GULPSF_ERROR("Self-test FAIL: could not prepare statement: {}", sqlite3_errmsg(db_));
    std::cerr << "[chain_txids] Self-test FAIL: could not prepare statement" << std::endl;
    return;
  }

  std::string test_txid;
  uint64_t expected_height = 0;
  if(sqlite3_step(stmt) == SQLITE_ROW)
  {
    const unsigned char *txid_ptr = sqlite3_column_text(stmt, 0);
    if(txid_ptr)
    {
      test_txid = reinterpret_cast<const char *>(txid_ptr);
      expected_height = static_cast<uint64_t>(sqlite3_column_int64(stmt, 1));
    }
  }
  sqlite3_finalize(stmt);

  if(!test_txid.empty())
  {
    auto result = lookup_txid(test_txid);
    if(result && result->height == expected_height)
    {
      GULPSF_INFO("Self-test PASS: txid lookup returned HIT with correct height {}", result->height);
      std::cerr << "[chain_txids] Self-test PASS: txid found at height " << result->height << std::endl;
    }
    else
    {
      GULPSF_ERROR("Self-test FAIL: txid lookup did not return expected result at height {}", expected_height);
      std::cerr << "[chain_txids] Self-test FAIL: txid not found at expected height" << std::endl;
    }
  }
  else
  {
    GULPSF_WARN("Self-test WARN: database is empty, skipping txid lookup test");
    std::cerr << "[chain_txids] Self-test WARN: database is empty" << std::endl;
  }

  // TEST 2: Verify that querying a non-existent transaction returns no result
  // Create a fake txid that should not exist in the database (64 zero hex characters)
  const std::string fake_txid(64, '0');
  auto fake_result = lookup_txid(fake_txid);
  if(!fake_result)
  {
    GULPS_INFO("Self-test PASS: fake txid lookup returned MISS as expected");
    std::cerr << "[chain_txids] Self-test PASS: fake txid returned MISS" << std::endl;
  }
  else
  {
    GULPS_ERROR("Self-test FAIL: fake txid lookup should have returned MISS");
    std::cerr << "[chain_txids] Self-test FAIL: fake txid should have returned MISS" << std::endl;
  }

  // TEST 3: Log the maximum height available in the database
  auto max_height = get_max_height();
  if(max_height)
  {
    GULPSF_INFO("chain_txids database max height: {}", *max_height);
    std::cerr << "[chain_txids] Database max height: " << *max_height << std::endl;
  }
  else
  {
    GULPS_ERROR("Failed to get max height from chain_txids database");
    std::cerr << "[chain_txids] ERROR: Failed to get max height" << std::endl;
  }

  GULPS_INFO("chain_txids database self-tests completed");
  std::cerr << "[chain_txids] Self-tests completed" << std::endl;
}

} // namespace daemonize

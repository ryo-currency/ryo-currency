#!/usr/bin/env python3
"""
Build a SQLite database of all confirmed transaction IDs from the Ryo blockchain.

FLOW & USAGE:
  1. This script connects to a running Ryo daemon (default: localhost:12211)
  2. For each block from start_height to end_height:
     - Fetch the block hash for that height
     - Fetch the full block data (includes txid list)
     - Extract all transaction IDs (txids) from that block
     - Insert each txid into the SQLite database with height and block_hash
  3. Uses upserts to safely handle re-runs and correct stale rows
  4. Prints progress every N blocks (configurable)
  5. Optionally: lookup a txid to find its height and block_hash
  6. Validates the final database with statistics

DATABASE SCHEMA:
  chain_txids (txid TEXT PRIMARY KEY, height INTEGER, block_hash TEXT)
  idx_chain_txids_height (index on height for fast range queries)

EXAMPLE USAGE:
  # Build database for all blocks up to height 1000000
  python3 build_txid_sqlite.py --end-height 1000000

  # Build from scratch to current tip with progress every 1000 blocks
  python3 build_txid_sqlite.py --auto-detect --progress-interval 1000

  # Lookup a txid in the database
  python3 build_txid_sqlite.py --lookup e1a1b2c3d4e5f6...

  # Validate existing database
  python3 build_txid_sqlite.py --validate-only
"""

import argparse
import json
import sqlite3
import sys
import time
import urllib.request
from datetime import datetime


# ============================================================================
# Configuration & RPC Setup
# ============================================================================

DEFAULT_RPC_HOST = "127.0.0.1"
DEFAULT_RPC_PORT = 12211
DEFAULT_START_HEIGHT = 0
DEFAULT_DB_PATH = "chain_txids.sqlite"
DEFAULT_PROGRESS_INTERVAL = 10000


def build_rpc_url(host: str, port: int) -> str:
    """Construct the daemon RPC base URL from host and port."""
    return f"http://{host}:{port}"


def json_rpc_url(rpc_url: str) -> str:
    """Return the daemon JSON-RPC endpoint URL."""
    return f"{rpc_url.rstrip('/')}/json_rpc"


def http_rpc_url(rpc_url: str, path: str) -> str:
    """Return a non-JSON-RPC daemon endpoint URL."""
    return f"{rpc_url.rstrip('/')}/{path.lstrip('/')}"


def rpc_json(method: str, params=None, rpc_url: str = None):
    """
    Send a JSON-RPC request to the Ryo daemon.

    Args:
        method: RPC method name (e.g., "get_block", "on_get_block_hash")
        params: Parameters for the method; usually a dict, sometimes a list
        rpc_url: Base RPC URL (http://host:port)

    Returns:
        Parsed JSON response from the daemon
    """
    if rpc_url is None:
        rpc_url = build_rpc_url(DEFAULT_RPC_HOST, DEFAULT_RPC_PORT)

    body = {
        "jsonrpc": "2.0",
        "id": "0",
        "method": method,
        "params": {} if params is None else params,
    }

    request = urllib.request.Request(
        json_rpc_url(rpc_url),
        data=json.dumps(body).encode(),
        headers={"Content-Type": "application/json"},
    )

    with urllib.request.urlopen(request, timeout=10) as response:
        result = json.loads(response.read().decode())
        if "error" in result and result["error"] is not None:
            raise RuntimeError(f"RPC Error: {result['error']}")
        return result.get("result", {})


def rpc_http_json(path: str, params: dict = None, rpc_url: str = None) -> dict:
    """
    Send a plain HTTP JSON request to the daemon.

    Ryo exposes some daemon calls, including get_height, as HTTP endpoints
    rather than JSON-RPC methods.
    """
    if rpc_url is None:
        rpc_url = build_rpc_url(DEFAULT_RPC_HOST, DEFAULT_RPC_PORT)

    request = urllib.request.Request(
        http_rpc_url(rpc_url, path),
        data=json.dumps(params or {}).encode(),
        headers={"Content-Type": "application/json"},
    )

    with urllib.request.urlopen(request, timeout=10) as response:
        result = json.loads(response.read().decode())
        status = result.get("status")
        if status and status != "OK":
            raise RuntimeError(f"HTTP RPC {path} returned status {status}")
        return result


def get_daemon_height(rpc_url: str) -> int:
    """Fetch the current daemon blockchain height."""
    # get_height is a plain HTTP JSON endpoint, not a JSON-RPC method.
    last_error = None
    for path in ("/get_height", "/getheight"):
        try:
            result = rpc_http_json(path, rpc_url=rpc_url)
            return int(result.get("height", 0))
        except Exception as exc:
            last_error = exc

    raise RuntimeError(f"failed to fetch daemon height: {last_error}")


def get_block_hash(height: int, rpc_url: str) -> str:
    """
    Fetch the block hash for a given height.

    Note: on_get_block_hash expects height as an array [height], not an object.
    This is a quirk of the Ryo RPC API.
    """
    block_hash = rpc_json("on_get_block_hash", [height], rpc_url=rpc_url)
    if not is_hex_hash(block_hash):
        raise RuntimeError(f"daemon returned invalid block hash for height {height}: {block_hash!r}")
    return block_hash


def get_block(block_hash: str, rpc_url: str) -> dict:
    """
    Fetch the full block data (including transaction list).

    Args:
        block_hash: The hash of the block to fetch

    Returns:
        Dict with keys like 'block_header', 'tx_hashes', 'miner_tx_hash', etc.
    """
    # get_block returns block_header, miner_tx_hash, tx_hashes, blob, and JSON.
    result = rpc_json("get_block", {"hash": block_hash}, rpc_url=rpc_url)
    return result


def is_hex_hash(value: str) -> bool:
    """Return True if value looks like a 32-byte hash encoded as hex."""
    return (
        isinstance(value, str)
        and len(value) == 64
        and all(char in "0123456789abcdefABCDEF" for char in value)
    )


def tx_hashes_from_block(block_data: dict) -> list:
    """Extract regular transaction hashes from a get_block response."""
    tx_hashes = block_data.get("tx_hashes")
    if tx_hashes is None and block_data.get("json"):
        decoded_block = json.loads(block_data["json"])
        tx_hashes = decoded_block.get("tx_hashes")

    if tx_hashes is None:
        return []
    if not isinstance(tx_hashes, list):
        raise RuntimeError(f"unexpected tx_hashes value: {tx_hashes!r}")

    for txid in tx_hashes:
        if not is_hex_hash(txid):
            raise RuntimeError(f"daemon returned invalid tx hash: {txid!r}")
    return tx_hashes


def extract_block_txids(block_data: dict, expected_height: int, expected_hash: str) -> list:
    """Validate one get_block response and return miner plus regular txids."""
    status = block_data.get("status")
    if status and status != "OK":
        raise RuntimeError(f"get_block returned status {status} for height {expected_height}")

    header = block_data.get("block_header") or {}
    block_hash = header.get("hash")
    block_height = header.get("height")
    if block_hash != expected_hash:
        raise RuntimeError(
            f"get_block hash mismatch at height {expected_height}: "
            f"expected {expected_hash}, got {block_hash}"
        )
    if int(block_height) != expected_height:
        raise RuntimeError(
            f"get_block height mismatch for {expected_hash}: "
            f"expected {expected_height}, got {block_height}"
        )

    miner_tx_hash = block_data.get("miner_tx_hash")
    if not is_hex_hash(miner_tx_hash):
        raise RuntimeError(f"daemon returned invalid miner tx hash at height {expected_height}: {miner_tx_hash!r}")

    return [miner_tx_hash] + tx_hashes_from_block(block_data)


# ============================================================================
# SQLite Database Setup & Operations
# ============================================================================

def init_database(db_path: str) -> sqlite3.Connection:
    """
    Initialize the SQLite database with the required schema.

    Creates the chain_txids table and index if they don't exist.
    Uses upserts to handle duplicate txids safely and correct stale rows.

    Args:
        db_path: Path to the SQLite database file

    Returns:
        sqlite3.Connection object
    """
    conn = sqlite3.connect(db_path)
    cursor = conn.cursor()

    # Create main table: txid as PRIMARY KEY ensures uniqueness
    cursor.execute("""
        CREATE TABLE IF NOT EXISTS chain_txids (
            txid TEXT PRIMARY KEY,
            height INTEGER NOT NULL,
            block_hash TEXT
        )
    """)

    # Create index on height for fast range queries (lookup by height)
    cursor.execute("""
        CREATE INDEX IF NOT EXISTS idx_chain_txids_height ON chain_txids(height)
    """)

    conn.commit()
    return conn


def insert_txid(conn: sqlite3.Connection, txid: str, height: int, block_hash: str) -> bool:
    """
    Insert a transaction ID into the database.

    Uses an upsert so duplicate txids don't cause errors and stale rows from
    older runs are corrected.

    Args:
        conn: Database connection
        txid: Transaction ID to insert
        height: Block height where this txid appears
        block_hash: Hash of the block containing this txid

    Returns:
        True if inserted or updated, False if error
    """
    cursor = conn.cursor()
    try:
        cursor.execute(
            """
            INSERT INTO chain_txids (txid, height, block_hash)
            VALUES (?, ?, ?)
            ON CONFLICT(txid) DO UPDATE SET
                height = excluded.height,
                block_hash = excluded.block_hash
            """,
            (txid, height, block_hash)
        )
        return True
    except Exception as e:
        print(f"ERROR inserting txid {txid}: {e}", file=sys.stderr)
        return False


def batch_insert_txids(conn: sqlite3.Connection, txids: list, height: int, block_hash: str) -> int:
    """
    Insert multiple transaction IDs in a batch (more efficient).

    Args:
        conn: Database connection
        txids: List of txids to insert
        height: Block height
        block_hash: Block hash

    Returns:
        Number of rows inserted or updated
    """
    cursor = conn.cursor()
    try:
        before_changes = conn.total_changes
        cursor.executemany(
            """
            INSERT INTO chain_txids (txid, height, block_hash)
            VALUES (?, ?, ?)
            ON CONFLICT(txid) DO UPDATE SET
                height = excluded.height,
                block_hash = excluded.block_hash
            WHERE
                chain_txids.height != excluded.height
                OR chain_txids.block_hash != excluded.block_hash
            """,
            [(txid, height, block_hash) for txid in txids],
        )
        conn.commit()
        return conn.total_changes - before_changes
    except Exception as e:
        print(f"ERROR during batch insert: {e}", file=sys.stderr)
        conn.rollback()
        return 0


def lookup_txid(conn: sqlite3.Connection, txid: str) -> dict:
    """
    Look up a transaction ID in the database.

    Args:
        conn: Database connection
        txid: Transaction ID to look up

    Returns:
        Dict with 'found', 'height', 'block_hash' keys
    """
    cursor = conn.cursor()
    cursor.execute(
        "SELECT height, block_hash FROM chain_txids WHERE txid = ?",
        (txid,)
    )
    row = cursor.fetchone()

    if row:
        return {
            "found": True,
            "txid": txid,
            "height": row[0],
            "block_hash": row[1]
        }
    else:
        return {
            "found": False,
            "txid": txid,
            "message": "Transaction ID not found in database"
        }


def validate_database(conn: sqlite3.Connection) -> dict:
    """
    Validate the database with statistics.

    Returns: Dict with min_height, max_height, total_txids, distinct_heights,
    and basic consistency checks.
    """
    cursor = conn.cursor()

    # Get total count
    cursor.execute("SELECT COUNT(*) FROM chain_txids")
    total_txids = cursor.fetchone()[0]

    # Get height range
    cursor.execute("SELECT MIN(height), MAX(height) FROM chain_txids")
    min_height, max_height = cursor.fetchone()

    # Get distinct heights
    cursor.execute("SELECT COUNT(DISTINCT height) FROM chain_txids")
    distinct_heights = cursor.fetchone()[0]

    cursor.execute("SELECT COUNT(*) FROM chain_txids WHERE block_hash IS NULL OR block_hash = ''")
    missing_block_hashes = cursor.fetchone()[0]

    cursor.execute("""
        SELECT COUNT(*)
        FROM chain_txids
        WHERE length(txid) != 64 OR length(block_hash) != 64
    """)
    invalid_hash_lengths = cursor.fetchone()[0]

    height_span = 0
    missing_heights = 0
    if min_height is not None and max_height is not None:
        height_span = max_height - min_height + 1
        missing_heights = height_span - distinct_heights

    return {
        "total_txids": total_txids,
        "min_height": min_height,
        "max_height": max_height,
        "distinct_heights": distinct_heights,
        "height_span": height_span,
        "missing_heights": missing_heights,
        "missing_block_hashes": missing_block_hashes,
        "invalid_hash_lengths": invalid_hash_lengths,
    }


# ============================================================================
# Block Processing & Main Build Loop
# ============================================================================

def build_txid_index(
    start_height: int,
    end_height: int,
    db_path: str,
    rpc_url: str,
    progress_interval: int = DEFAULT_PROGRESS_INTERVAL,
):
    """
    Main loop to fetch blocks and build the txid index.

    For each height from start_height to end_height (inclusive):
      1. Fetch the block hash for that height
      2. Fetch the full block data
      3. Extract all transaction IDs from the block
      4. Insert txids into the database

    Args:
        start_height: Starting block height (inclusive)
        end_height: Ending block height (inclusive)
        db_path: Path to SQLite database
        rpc_url: Base RPC URL (http://host:port)
        progress_interval: Print progress every N blocks
    """
    conn = init_database(db_path)
    start_time = time.time()
    total_txids_seen = 0
    total_rows_changed = 0

    print(f"\n{'='*70}")
    print(f"Building txid index: blocks {start_height} to {end_height}")
    print(f"Database: {db_path}")
    print(f"Progress interval: {progress_interval} blocks")
    print(f"{'='*70}\n")

    try:
        for height in range(start_height, end_height + 1):
            # Step 1: Fetch block hash for this height
            block_hash = get_block_hash(height, rpc_url)

            # Step 2: Fetch full block data
            block_data = get_block(block_hash, rpc_url)

            # Step 3: Validate and extract miner tx plus regular txids.
            txids = extract_block_txids(block_data, height, block_hash)
            total_txids_seen += len(txids)

            # Step 4: Insert all txids for this block
            changed = batch_insert_txids(conn, txids, height, block_hash)
            total_rows_changed += changed

            # Progress reporting
            if progress_interval > 0 and (height - start_height + 1) % progress_interval == 0:
                elapsed = time.time() - start_time
                blocks_processed = height - start_height + 1
                rate = blocks_processed / elapsed if elapsed > 0 else 0
                eta_remaining = (end_height - height) / rate if rate > 0 else 0
                print(
                    f"[{datetime.now().strftime('%H:%M:%S')}] "
                    f"Height {height:>10} | "
                    f"TXIDs seen: {total_txids_seen:>10} | "
                    f"Rows changed: {total_rows_changed:>10} | "
                    f"Elapsed: {int(elapsed):>4}s | "
                    f"Rate: {rate:>6.1f} blocks/s | "
                    f"ETA: {int(eta_remaining):>4}s"
                )

    except KeyboardInterrupt:
        print("\n\nInterrupted by user. Saving progress...", file=sys.stderr)
    except Exception as e:
        print(f"\n\nERROR during build: {e}", file=sys.stderr)
        raise
    finally:
        conn.close()

    # Final summary
    elapsed_total = time.time() - start_time
    print(f"\n{'='*70}")
    print(f"Build complete!")
    print(f"Total time: {int(elapsed_total)} seconds")
    print(f"Total txids seen: {total_txids_seen}")
    print(f"Rows inserted/updated: {total_rows_changed}")
    print(f"Blocks processed: {end_height - start_height + 1}")
    print(f"{'='*70}\n")


# ============================================================================
# CLI & Main Entry Point
# ============================================================================

def main():
    parser = argparse.ArgumentParser(
        description="Build a SQLite database of confirmed transaction IDs from the Ryo blockchain",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
EXAMPLES:
  # Build txid index up to block 1000000
  python3 build_txid_sqlite.py --end-height 1000000

  # Build to current tip, print progress every 5000 blocks
  python3 build_txid_sqlite.py --auto-detect --progress-interval 5000

  # Lookup a transaction ID
  python3 build_txid_sqlite.py --lookup abc123def456...

  # Validate an existing database
  python3 build_txid_sqlite.py --validate-only
        """
    )

    # RPC connection options
    parser.add_argument(
        "--rpc-host",
        default=DEFAULT_RPC_HOST,
        help=f"Daemon RPC host (default: {DEFAULT_RPC_HOST})"
    )
    parser.add_argument(
        "--rpc-port",
        type=int,
        default=DEFAULT_RPC_PORT,
        help=f"Daemon RPC port (default: {DEFAULT_RPC_PORT})"
    )

    # Height range options
    parser.add_argument(
        "--start-height",
        type=int,
        default=DEFAULT_START_HEIGHT,
        help=f"Starting block height (default: {DEFAULT_START_HEIGHT})"
    )
    parser.add_argument(
        "--end-height",
        type=int,
        help="Ending block height (required unless using --lookup or --validate-only)"
    )
    parser.add_argument(
        "--auto-detect",
        action="store_true",
        help="Auto-detect end height as current daemon tip (use with caution on busy networks)"
    )

    # Database options
    parser.add_argument(
        "--db-path",
        default=DEFAULT_DB_PATH,
        help=f"Path to SQLite database (default: {DEFAULT_DB_PATH})"
    )
    parser.add_argument(
        "--progress-interval",
        type=int,
        default=DEFAULT_PROGRESS_INTERVAL,
        help=f"Print progress every N blocks (default: {DEFAULT_PROGRESS_INTERVAL})"
    )

    # Special modes
    parser.add_argument(
        "--lookup",
        type=str,
        help="Lookup a transaction ID in the database and exit"
    )
    parser.add_argument(
        "--validate-only",
        action="store_true",
        help="Validate existing database and print statistics, then exit"
    )

    args = parser.parse_args()

    # Build RPC base URL
    rpc_url = build_rpc_url(args.rpc_host, args.rpc_port)

    # -----------------------------------------------------------------------
    # LOOKUP MODE: Find a txid in the database
    # -----------------------------------------------------------------------
    if args.lookup:
        print(f"\nLooking up txid: {args.lookup}\n")
        try:
            conn = sqlite3.connect(args.db_path)
            result = lookup_txid(conn, args.lookup)
            conn.close()

            if result["found"]:
                print(f"✓ Found!")
                print(f"  Txid:       {result['txid']}")
                print(f"  Height:     {result['height']}")
                print(f"  Block Hash: {result['block_hash']}")
            else:
                print(f"✗ {result['message']}")
            print()
        except Exception as e:
            print(f"ERROR: {e}", file=sys.stderr)
            sys.exit(1)
        return

    # -----------------------------------------------------------------------
    # VALIDATE MODE: Print database statistics
    # -----------------------------------------------------------------------
    if args.validate_only:
        print(f"\nValidating database: {args.db_path}\n")
        try:
            conn = sqlite3.connect(args.db_path)
            stats = validate_database(conn)
            conn.close()

            print("Database Statistics:")
            print(f"  Total TXIDs:        {stats['total_txids']:>10}")
            print(f"  Min Height:         {str(stats['min_height']):>10}")
            print(f"  Max Height:         {str(stats['max_height']):>10}")
            print(f"  Distinct Heights:   {stats['distinct_heights']:>10}")
            print(f"  Height Span:        {stats['height_span']:>10}")
            print(f"  Missing Heights:    {stats['missing_heights']:>10}")
            print(f"  Missing Block Hash: {stats['missing_block_hashes']:>10}")
            print(f"  Invalid Hash Len:   {stats['invalid_hash_lengths']:>10}")
            print()

            if stats['total_txids'] > 0 and stats['distinct_heights'] > 0:
                avg_txids_per_block = stats['total_txids'] / stats['distinct_heights']
                print(f"  Avg TXIDs/Block:    {avg_txids_per_block:>10.2f}")
            print()
        except Exception as e:
            print(f"ERROR: {e}", file=sys.stderr)
            sys.exit(1)
        return

    # -----------------------------------------------------------------------
    # BUILD MODE: Fetch blocks and build txid index
    # -----------------------------------------------------------------------

    if args.start_height < 0:
        print(f"ERROR: start-height ({args.start_height}) must be >= 0", file=sys.stderr)
        sys.exit(1)

    # Determine end height
    end_height = args.end_height
    if args.auto_detect:
        print("Auto-detecting daemon tip height...")
        try:
            daemon_height = get_daemon_height(rpc_url)
            # Daemon height is 1-indexed; use height-1 to get the last confirmed block
            end_height = daemon_height - 1
            print(f"Detected current height: {end_height}\n")
        except Exception as e:
            print(f"ERROR: Failed to auto-detect height: {e}", file=sys.stderr)
            sys.exit(1)

    if end_height is None:
        print("ERROR: --end-height is required (or use --auto-detect)", file=sys.stderr)
        parser.print_help()
        sys.exit(1)

    if end_height < 0:
        print(f"ERROR: end-height ({end_height}) must be >= 0", file=sys.stderr)
        sys.exit(1)

    if end_height < args.start_height:
        print(
            f"ERROR: end-height ({end_height}) must be >= start-height ({args.start_height})",
            file=sys.stderr
        )
        sys.exit(1)

    # Validate daemon is reachable
    try:
        daemon_height = get_daemon_height(rpc_url)
        print(f"Daemon reachable. Current height: {daemon_height - 1}\n")
    except Exception as e:
        print(
            f"ERROR: Cannot reach daemon at {rpc_url}: {e}",
            file=sys.stderr
        )
        sys.exit(1)

    # Start the build process
    try:
        build_txid_index(
            start_height=args.start_height,
            end_height=end_height,
            db_path=args.db_path,
            rpc_url=rpc_url,
            progress_interval=args.progress_interval,
        )

        # Print validation stats after build
        print("Running final validation...\n")
        conn = sqlite3.connect(args.db_path)
        stats = validate_database(conn)
        conn.close()

        print("Final Database Statistics:")
        print(f"  Total TXIDs:        {stats['total_txids']:>10}")
        print(f"  Min Height:         {str(stats['min_height']):>10}")
        print(f"  Max Height:         {str(stats['max_height']):>10}")
        print(f"  Distinct Heights:   {stats['distinct_heights']:>10}")
        print(f"  Height Span:        {stats['height_span']:>10}")
        print(f"  Missing Heights:    {stats['missing_heights']:>10}")
        print(f"  Missing Block Hash: {stats['missing_block_hashes']:>10}")
        print(f"  Invalid Hash Len:   {stats['invalid_hash_lengths']:>10}")
        print()

    except Exception as e:
        print(f"FATAL: {e}", file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    main()

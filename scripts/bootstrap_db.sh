#!/usr/bin/env bash
set -euo pipefail

DB_PATH="${1:-./kalanet.db}"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

sqlite3 "$DB_PATH" < "$SCRIPT_DIR/bootstrap_db.sql"
echo "Database bootstrapped at: $DB_PATH"

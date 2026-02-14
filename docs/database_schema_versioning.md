# KalaNet Database Schema & Versioning

## Database file
Default SQLite file: `kalanet.db` in the server executable directory.

## Schema ownership
- `SqliteUserRepository`: `users`
- `SqliteAdRepository`: `ads`, `ad_status_history`, `schema_migrations`
- `SqliteCartRepository`: `cart_items`
- `SqliteWalletRepository`: `wallets`, `transaction_ledger`

## Versioning model
Advertisement schema uses incremental migrations recorded in `schema_migrations`.
Current version: **5**.

Migration order:
1. `ads` table
2. `ad_status_history`
3. index on `(status, created_at)`
4. index on `seller_username`
5. index on `ad_status_history(ad_id, changed_at)`

## Bootstrap
Use scripts in `scripts/`:
- `bootstrap_db.sh` (runs sqlite3 against target file)
- `bootstrap_db.sql` (idempotent schema DDL)

Example:
```bash
./scripts/bootstrap_db.sh ./kalanet.db
```

## Transaction boundaries
- Wallet top-up runs in one DB transaction: wallet update + ledger insert.
- Checkout runs in one DB transaction: ad checks, buyer debit, seller credit, ad sold-state update, ledger inserts, cart cleanup.
- Checkout rollback is automatic on any failure path.

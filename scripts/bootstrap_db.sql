PRAGMA foreign_keys = ON;

CREATE TABLE IF NOT EXISTS users (
    id           INTEGER PRIMARY KEY AUTOINCREMENT,
    full_name    TEXT NOT NULL,
    username     TEXT NOT NULL UNIQUE,
    phone        TEXT NOT NULL,
    email        TEXT NOT NULL UNIQUE,
    passwordHash TEXT NOT NULL,
    role         TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS schema_migrations (
    version INTEGER PRIMARY KEY,
    applied_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE IF NOT EXISTS ads (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    title TEXT NOT NULL,
    description TEXT NOT NULL,
    category TEXT NOT NULL,
    price_tokens INTEGER NOT NULL CHECK (price_tokens > 0),
    seller_username TEXT NOT NULL,
    image_bytes BLOB,
    status TEXT NOT NULL,
    created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE IF NOT EXISTS ad_status_history (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    ad_id INTEGER NOT NULL,
    previous_status TEXT,
    new_status TEXT NOT NULL,
    changed_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
    reason TEXT,
    FOREIGN KEY(ad_id) REFERENCES ads(id) ON DELETE CASCADE
);

CREATE INDEX IF NOT EXISTS idx_ads_status_created_at ON ads(status, created_at DESC);
CREATE INDEX IF NOT EXISTS idx_ads_seller_username ON ads(seller_username);
CREATE INDEX IF NOT EXISTS idx_ad_status_history_ad_id_changed_at ON ad_status_history(ad_id, changed_at DESC);

CREATE TABLE IF NOT EXISTS cart_items (
    username TEXT NOT NULL,
    ad_id INTEGER NOT NULL,
    created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY(username, ad_id)
);

CREATE TABLE IF NOT EXISTS wallets (
    username TEXT PRIMARY KEY,
    balance_tokens INTEGER NOT NULL DEFAULT 0
);

CREATE TABLE IF NOT EXISTS transaction_ledger (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    username TEXT NOT NULL,
    type TEXT NOT NULL,
    amount_tokens INTEGER NOT NULL,
    balance_after INTEGER NOT NULL,
    ad_id INTEGER,
    counterparty TEXT,
    created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP
);

CREATE INDEX IF NOT EXISTS idx_transaction_ledger_user_created
ON transaction_ledger(username, created_at DESC);

INSERT OR IGNORE INTO schema_migrations(version) VALUES (1);
INSERT OR IGNORE INTO schema_migrations(version) VALUES (2);
INSERT OR IGNORE INTO schema_migrations(version) VALUES (3);
INSERT OR IGNORE INTO schema_migrations(version) VALUES (4);
INSERT OR IGNORE INTO schema_migrations(version) VALUES (5);

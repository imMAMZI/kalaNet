# KalaNet

KalaNet is a client/server marketplace project for the **Advanced Programming** course at Isfahan University of Technology.
It is a desktop application ecosystem built with **C++23 + Qt6 + SQLite** where users can sign up, post ads, browse approved ads, manage a cart, pay with wallet tokens, and use discount codes.

---

## 1) Project Overview

KalaNet is split into three CMake targets:

- **`common`**: shared domain models and wire protocol types/serialization.
- **`serverProject`**: Qt-based TCP server, repositories, business services, and an admin/server console UI.
- **`clientProject`**: Qt Widgets desktop client with Login/Signup, Shop, Cart, Profile, New Ad, and Guide pages.

Communication is done through a custom framed protocol over TCP (`127.0.0.1:8080`) with strongly typed command enums.

---

## 2) Main Features

### Authentication & Session
- Signup and login flow with server-side validation.
- Password hashing/verification.
- Session token issuance + refresh + logout.
- CAPTCHA challenge endpoint used in login and wallet top-up flows.

### Marketplace Core
- Create new ads (title, description, category, price, optional image).
- Ads are submitted in **pending** state for moderation.
- Admin moderation path for approving/rejecting ads.
- Shop listing and ad detail retrieval.
- Cart add/remove/list/clear.
- Checkout (`Buy`) for all cart items in one transaction.

### Wallet & Discount Codes
- Wallet balance and token top-up.
- Ledger-backed transaction history.
- Discount code validation with support for:
  - percentage/fixed discounts,
  - max discount cap,
  - minimum subtotal,
  - usage limits,
  - expiry and active flags.

### Profile & History
- Profile update (and password change path).
- Personal history including:
  - posted ads,
  - sold ads,
  - purchased ads,
  - wallet changes,
  - transaction list.

### Admin / Server-side Tools
- Server console window with request logs and filters.
- Pending ads window.
- Discount code manager window.
- Users information window.

---

## 3) Repository Structure

```text
kalaNet/
├─ CMakeLists.txt
├─ common/                    # Shared models + protocol
├─ server/                    # TCP server, services, repositories, admin UI
├─ client/                    # End-user desktop application
├─ scripts/
│  ├─ bootstrap_db.sql        # Idempotent schema bootstrap
│  └─ bootstrap_db.sh         # Helper to initialize DB with sqlite3
└─ docs/
   └─ database_schema_versioning.md
```

---

## 4) Technology Stack

- **Language:** C++23
- **GUI/Network/DB framework:** Qt6 (`Core`, `Gui`, `Widgets`, `Network`, `Sql`)
- **Database:** SQLite
- **Build system:** CMake (3.21+)

---

## 5) Build Instructions

> The project uses out-of-source CMake builds.

### Prerequisites
- CMake 3.21+
- C++23-capable compiler (GCC/Clang/MSVC)
- Qt6 development packages:
  - Core, Gui, Widgets, Network, Sql
- SQLite runtime (and `sqlite3` CLI if you want to run bootstrap scripts manually)

### Build

```bash
cmake -S . -B build
cmake --build build -j
```

Generated binaries (default names):
- `build/server/serverProject`
- `build/client/clientProject`

---

## 6) Run Instructions

### 1. Start server

```bash
./build/server/serverProject
```

- Server listens on TCP port **8080** by default.
- Database file defaults to `kalanet.db` in the executable working directory.

### 2. Start client

```bash
./build/client/clientProject
```

- Client connects to `127.0.0.1:8080`.

---

## 7) Database Bootstrap

The server repositories initialize required tables automatically, but you can also pre-bootstrap manually.

```bash
./scripts/bootstrap_db.sh ./kalanet.db
```

This applies idempotent DDL from `scripts/bootstrap_db.sql` and seeds default discount codes (`OFF10`, `OFF20`).

---

## 8) Typical User Flow

1. Open client and sign up.
2. Login (with CAPTCHA).
3. Post a new ad from **New Ad** page.
4. Admin reviews pending ads and approves them.
5. Buyers browse approved ads in **Shop** and add to cart.
6. Buyers open **Cart**, optionally apply discount code, and checkout.
7. Users review purchases and wallet history in **Profile**.

---

## 9) Notes for Developers

- The project uses a shared `common` protocol layer to keep server/client command contracts aligned.
- Command routing lives in `server/protocol/request_dispatcher.*`.
- Business logic is divided into services (`auth`, `ads`, `cart`, `wallet`) with SQLite-backed repository implementations.
- Database schema versioning notes are in `docs/database_schema_versioning.md`.

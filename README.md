# Gymtrac

`Gymtrac` is a gym management system, a `group project` for `CSE115L` course of North South University. The project is developed using C following C11 standards. This is a complete `CLI only` project, with no GUI.

## Disclaimer About Stupidity

> This is a demo project, and it does not care about engineering!

CSE115L is the very first major course a student takes in CSE, and this project exists purely to practice the concepts taught in class. It does not aim for prod-grade code; deliberately stupid but working solutions beat heavily engineered ones here.

> Password hashing here stops a curious classmate, not an attacker with a GPU.

This project uses a polynomial hash function (similar to Java's `String.hashCode()`) for password storage. This is **not** a cryptographically secure hash and should **never** be used in production. It is used here due to CSE115L project constraints that prohibit dynamic memory allocation, bitwise operations, and proper hashing libraries (e.g., SHA-256, bcrypt).

> The PRNG is xoshiro128**, great for dice rolls, useless against attackers.

This project uses the [xoshiro128**](https://prng.di.unimi.it/xoshiro128starstar.c) pseudorandom number generator by David Blackman and Sebastiano Vigna for random number generation. This is **not** a cryptographically secure PRNG and should **never** be used in production. It is used here as a higher-quality alternative to the standard library `rand()` function. The implementation avoids bitwise operators by achieving equivalent computational results through arithmetic operations. Reference: <https://prng.di.unimi.it/>

> No malloc, no database, just fixed arrays with ceilings written in settings.h.

Dynamic memory allocation (`malloc`/`realloc`/`free`) is prohibited in this project, so every record table lives in a static fixed-size array whose size must be decided at compile time. The `MAX_*_RECORDS` macros in `settings.h` provide those sizes: derived caps like `MAX_GYM_MEMBERS` bound the user tables, and event-table caps like `MAX_SUSPENSION_RECORDS` or `MAX_PAYMENT_RECORDS` bound records that accumulate over a member's lifetime. When a table reaches its cap, creating further records is rejected instead of silently dropped.

> split() (string_util.c:34) knows one buffer width. Hand it another and the compiler yells at you.

`split()` (string_util.c:34) takes its output as `char parts[][FIELD_BUFFER_SIZE]`, not the more flexible `char *parts[]`. This is deliberate. The field width is controlled centrally through the `FIELD_BUFFER_SIZE` macro in `settings.h`, and because dynamic memory allocation is prohibited, every caller already declares fixed `parts[][FIELD_BUFFER_SIZE]` arrays anyway. Passing the real 2D array directly removes the pointer-map boilerplate callers previously needed, and the compiler now rejects any buffer whose row width differs from `FIELD_BUFFER_SIZE` instead of letting a caller misreport capacities and overflow rows silently. The trade-off: `split()` can only split into buffers of that one width, so it cannot be reused for arbitrary-sized field buffers.

## Project Brief

- User authentication and authorization. Login with `username` + password; passwords are stored as salted hashes.
- 3 user record types: System Administrator (pre-existed), Branch Staff (a `role` field distinguishes Branch Manager from Branch Trainer), and Gym Member.
- Branches are a simple list of branch names. Every staff member and gym member belongs to exactly one existing branch.
- Each branch's staff and member counts are capped by `MAX_MANAGERS_PER_BRANCH`, `MAX_TRAINERS_PER_BRANCH`, and `MAX_MEMBERS_PER_BRANCH` macros defined in `settings.h`. Derived global caps (`MAX_BRANCH_MANAGERS`, `MAX_TRAINERS`, `MAX_GYM_MEMBERS`) bound the static arrays that store all records.
- A newly self-registered gym member has status `on_hold` until a branch manager approves them to `active`.
- Each member subscribes to a plan (`payable_amount` + `interval_days`), which drives their fee amount and payment interval.
- Digital payments are recorded by the member directly. Cash payments are handed to a branch trainer, who records them directly (no approval request).
- Each member can view their own payment history.
- A completed payment reduces a member's `due_amount` (clamped at 0) and updates their `last_payment_date`; the next due date is `last_payment_date + interval_days`.
- Each member is automatically suspended if their dues stay unpaid past the due date plus a grace period. A manager can unsuspend them after they pay their dues.
- Each member can view their own profile and submit profile-edit requests (name, phone, email, branch, username), which branch staff approve or reject.
- `Lost & Found`: members report lost or found items; branch staff can view and mark them as resolved.
- Suspensions and requests:
  - Only branch managers can approve a member (`on_hold` to `active`), suspend, or unsuspend them directly.
  - Branch trainers can only _request_ a member's status change (`membership_status_change_request_t`), and only branch managers resolve those requests.
  - Members request plan changes (`subscription_plan_change_request_t`) and profile edits (`profile_edit_request_t`); branch staff approve or reject them.
  - Every suspension has a mandatory `reason`, stored in a dedicated suspension record with its date (and an optional unsuspension date).
- Deletion rules:
  - A branch can only be deleted when no staff or member is assigned to it (`ensure_branch_has_no_users()` (branch.c:92)).
  - Branch staff and gym members can be deleted; a member with outstanding dues is protected and the system administrator account has no delete path.
- Access control:
  - Members see only their own data.
  - Branch managers and trainers see only their own branch's resources (members, payments, requests, lost & found).
  - The system administrator can perform every operation.
  - Only the system administrator can create new branches.
  - The system administrator can create users of any type (including branch managers); gym members self-register and stay `on_hold` until their branch manager approves them.

## TODO

[AI Generated]

Work tracked per item; commit each with a Conventional Commit message (`feat:`, `fix:`, `chore:`, `test:`, `docs:`).

### Scaffold

- [x] Add shared `src/types.h` defining every record struct plus `session_t` (fixed-size `char` arrays for strings), the `typedef unsigned char` enum aliases (`user_role_t` included) with `#define` constants, and `BRANCH_COUNT_MAX` in `settings.h` (already created) - `chore:`
- [x] Add `run.sh` and `run.bat` build scripts that compile `src/` with `-std=c11 -Wall -Wextra -pedantic -g` into `build/`, plus a `.gitignore` ignoring `data/` and `build/` so generated artifacts are never tracked - `chore:`

### Utils

- [x] String helpers that trim whitespace, split on a delimiter, parse numbers safely, normalize case, and sanitize field values so every module handles raw text predictably - `feat:`
- [x] File helpers that read/write one record per line, so persisted data stays clean and round-trips reliably - `feat:`
- [x] Unit tests covering trim/split/parse/case/sanitize behavior and file read/write round-trip - `test:`
- [x] Input wrappers around `fgets()` and `scanf()` that validate and cap input, so no buffer overflow or malformed value reaches the logic - `feat:`
- [x] Unit tests covering the input wrappers: line capping with overflow drain, and rejection of non-numeric input, zero, and negatives - `test:`
- [x] Update `settings.h`: resize `PASSWORD_HASH_BUFFER_SIZE` from 130 to 40, add `SALT_BUFFER_SIZE` (16) and `HASH_STRING_BUFFER_SIZE` (32) macros - `chore:`
- [x] Hash utility (`src/utils/hash.[ch]`): polynomial hash function (`h = 31 * h + c`, similar to Java's `String.hashCode()`), 15-char random salt generation via `rand()` seeded by `time(NULL)`, `mix_salt` (sandwich password between salt halves), `hash_value_to_string` / `parse_hash_value` conversions, and `compare_hash` equality check. Demo only, not cryptographically secure. - `feat:`
- [x] Auth module (`src/modules/auth.[ch]`): `hash_password` (generate salt, mix, hash, store salt+hash string) and `verify_password` (extract salt, re-hash, compare). Builds on hash utility for credential storage. - `feat:`
- [x] Unit tests for hash utility: known polynomial hash vectors, salt generation length and character set, `mix_salt` sandwich output, `hash_value_to_string` / `parse_hash_value` round-trip, `compare_hash` equality - `test:`
- [x] Unit tests for auth module: `hash_password` produces valid stored format, `verify_password` accepts correct and rejects wrong passwords - `test:`
- [x] Date helpers built on a custom `datetime_t` (year/month/day/hour/minute/second ints) that converts to and from epoch seconds with loop-based calendar math, no `localtime_r`/`localtime_s`, plus `add_months()` (datetime_utils.c:161) handling month-end, `add_days()` (datetime_utils.c:155), formatting/parsing `yyyy-mm-dd hh:mm:ss` - `feat:`
- [x] Unit tests covering date round-trips, leap years, and month-end arithmetic - `test:`
- [x] Derive `FIELD_DELIMITER` from a `FIELD_DELIMITER_STRING` literal so record format strings concatenate the shared macro instead of hardcoding the delimiter byte, with a one-character startup check in `main` - `chore:`

### Modules

- [x] Add `DATA_DIRECTORY` macro and per-file `*_FILE_PATH` macros in `settings.h` so every module resolves data files through one shared path convention - `feat:`
- [x] Branch module that loads/lists branch names and validates existence before anyone is assigned, so users can never attach to a nonexistent branch - `feat:`
- [x] Unit tests covering branch listing and existence validation - `test:`
- [x] User module that creates/fetches each role with auto-incremented ids, enforces globally unique usernames across all roles, and enforces per-branch capacity limits via `branch_manager_count()` (user.c:680), `branch_trainer_count()` (user.c:692), `branch_member_count()` (user.c:704) - `feat:`
- [x] Branch deletion guarded by `ensure_branch_has_no_users()` (branch.c:92), so a branch with assigned staff or members can never be removed - `feat:`
- [x] User deletion for branch staff and gym members; indebted members are blocked from deletion, and sysadmins have no delete path - `feat:`
- [x] Unit tests covering both delete policies, memory+disk round-trips after deletion, and username reuse once a record is gone - `test:`
- [x] Unit tests covering per-role CRUD, cross-role username uniqueness, and per-branch capacity enforcement - `test:`
- [x] Auth module that verifies username + password against stored salted polynomial hashes on login and clears the session on logout, so only verified users get in - `feat:`
- [x] Session module tracking role, user id, username, and branch per logged-in user, with predicates that gate actions by role and branch scope - `feat:`
- [x] Unit tests covering session predicates and branch-scope checks - `test:`
- [x] Member lifecycle: approve `on_hold` members to active, suspend/unsuspend with a mandatory reason (recording dated suspension records), and auto-suspend members with 90+ days unpaid dues - `feat:`
- [x] Unit tests covering approval, suspension records with optional unsuspension date, and the auto-suspend sweep - `test:`
- [x] Branch rename/update: renaming a branch requires cascading updates to all staff and member records referencing it - `feat:`
- [x] Unit tests covering branch rename persistence and the cascade into staff and member records - `test:`
- [x] Payment module: record digital payments instantly, let trainers record cash payments directly, then reduce `due_amount` (clamped at 0) and update `last_payment_date` - `feat:`
- [x] Unit tests covering digital payment, trainer-recorded cash payment, due clamping, and date advancement - `test:`
- [ ] Lost & Found module where members report lost/found items and managers/trainers list and resolve their branch's items - `feat:`
- [ ] Unit tests covering item reporting and resolution - `test:`
- [ ] Menu module with a login/register flow and one menu per role exposing only that role's allowed actions - `feat:`
- [ ] `main.c` that boots the app, seeds the default admin on first run, runs the auto-suspend sweep, and dispatches to the login flow - `feat:`

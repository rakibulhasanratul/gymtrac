# Gymtrac

`Gymtrac` is a gym management system, a `group project` for `CSE115L` course of North South University. The project is developed using C following C11 standards. This is a complete `CLI only` project, with no GUI.

## Brief Features
- User authentication and authorization. Login with `username` + password; passwords are stored as salted hashes.
- 3 user record types: System Administrator (pre-existed), Branch Staff (a `role` field distinguishes Branch Manager from Branch Trainer), and Gym Member.
- Branches are a simple list of branch names. Every staff member and gym member belongs to exactly one existing branch.
- Each branch has exactly one branch manager (a staff member with the manager role) and can have multiple trainers and multiple members.
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
- Access control:
  - Members see only their own data.
  - Branch managers and trainers see only their own branch's resources (members, payments, requests, lost & found).
  - The system administrator can perform every operation.
  - Only the system administrator can create new branches.
  - The system administrator can create users of any type (including branch managers); gym members self-register and stay `on_hold` until their branch manager approves them.

## TODO

Work tracked per item; commit each with a Conventional Commit message (`feat:`, `fix:`, `chore:`, `test:`, `docs:`).

### Scaffold
- [x] Add shared `src/types.h` defining every record struct plus `session_t` (fixed-size `char` arrays for strings), the `typedef unsigned char` enum aliases (`user_role_t` included) with `#define` constants, and the `BRANCH_COUNT_MAX` cap (already created) - `chore:`
- [x] Add `run.sh` and `run.bat` build scripts that compile `src/` with `-std=c11 -Wall -Wextra -pedantic -g` into `build/`, plus a `.gitignore` ignoring `data/` and `build/` so generated artifacts are never tracked - `chore:`

### Utils
- [ ] String helpers that trim whitespace, split on a delimiter, parse numbers safely, and normalize case so every module handles raw text predictably - `feat:`
- [ ] File helpers that read/write one record per line with control characters stripped, so persisted data stays clean and round-trips reliably - `feat:`
- [ ] Unit tests covering trim/split/parse/case behavior and file read/write round-trip - `test:`
- [ ] Input wrappers around `fgets()` and `scanf()` that validate and cap input, so no buffer overflow or malformed value reaches the logic - `feat:`
- [ ] SHA-256 password hashing with a per-password random salt, so credentials are never stored in plaintext and identical passwords differ on disk - `feat:`
- [ ] Unit tests using known SHA-256 vectors, plus salt generation and verify-password round-trips - `test:`
- [ ] Date helpers that convert `time_t` to `yyyy-mm-dd` and back (day-normalized), with `add_months()` handling month-end, so due dates and suspensions compute correctly - `feat:`
- [ ] Unit tests covering date round-trips, leap years, and month-end arithmetic - `test:`

### Modules
- [ ] Config module that resolves the data directory, builds file paths, and auto-creates the directory so all modules share one consistent storage location - `feat:`
- [ ] Branch module that loads/lists branch names and validates existence before anyone is assigned, so users can never attach to a nonexistent branch - `feat:`
- [ ] Unit tests covering branch listing and existence validation - `test:`
- [ ] User module that creates/fetches each role with auto-incremented ids, enforces globally unique usernames across all roles, and enforces one manager per branch - `feat:`
- [ ] Unit tests covering per-role CRUD, cross-role username uniqueness, and the one-manager-per-branch rule - `test:`
- [ ] Auth module that verifies username + password against stored salted hashes on login and clears the session on logout, so only verified users get in - `feat:`
- [ ] Member lifecycle: approve `on_hold` members to active, suspend/unsuspend with a mandatory reason (recording dated suspension records), and auto-suspend members with 90+ days unpaid dues - `feat:`
- [ ] Unit tests covering approval, suspension records with optional unsuspension date, and the auto-suspend sweep - `test:`
- [ ] Payment module: record digital payments instantly, let trainers record cash payments directly, then reduce `due_amount` (clamped at 0) and update `last_payment_date` - `feat:`
- [ ] Unit tests covering digital payment, trainer-recorded cash payment, due clamping, and date advancement - `test:`
- [ ] Lost & Found module where members report lost/found items and managers/trainers list and resolve their branch's items - `feat:`
- [ ] Unit tests covering item reporting and resolution - `test:`
- [ ] Session module tracking role, user id, username, and branch per logged-in user, with predicates that gate actions by role and branch scope - `feat:`
- [ ] Unit tests covering session predicates and branch-scope checks - `test:`
- [ ] Menu module with a login/register flow and one menu per role exposing only that role's allowed actions - `feat:`
- [ ] `main.c` that boots the app, seeds the default admin on first run, runs the auto-suspend sweep, and dispatches to the login flow - `feat:`

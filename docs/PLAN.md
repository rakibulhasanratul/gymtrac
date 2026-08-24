# Gymtrac - Implementation Plan

## Overview

Gymtrac is a CLI-only gym management system for CSE115L (North South University), written in C11. Persistence uses plain text files under `data/` (no databases). The system is built as small single-purpose modules and utilities, compiled directly with `gcc` through shell/batch scripts (`make` is prohibited). Code uses only common standard C headers and primitive data types; enumerations are `#define` constants backed by `typedef unsigned char` aliases, not bare `int` literals.

## Build & Run

- `./run.sh` compiles the main program into `build/gymtrac` and runs it.
- `./run.sh test` compiles the test runner into `build/test_runner` and runs it.
- Windows: `run.bat` and `run.bat test`.
- Compiler flags: `-std=c11 -Wall -Wextra -pedantic -g`.
- `make` is not used; `gcc` is invoked directly from the scripts.
- Repo hygiene is part of the scaffold: `run.sh`, `run.bat`, and `.gitignore` (ignoring `data/` and `build/`) are created up front.

## Data Storage

Field delimiter is `|` (pipe). Input is sanitized to strip control characters and the delimiter from field values before writing, so records stay unambiguous with no escaping logic. One record per line. Passwords are stored as `salt:hash_value` strings (polynomial hash, similar to Java's `String.hashCode()`, demo only -- not cryptographically secure; salted so identical passwords do not produce identical stored values). The salt is 16 alphanumeric characters; the hash is a decimal `unsigned int` (max 10 digits). The hash utility provides `generate_salt`, `polynomial_hash`, and `mix_salt`; the auth module composes these into `hash_password` and `verify_password`. Time fields use `datetime_t` (`year`, `month`, `day`, `hour`, `minute`, `second` as plain ints) and persist as one epoch-second number each; `datetime_utils` converts both ways with loop-based calendar math, no `localtime_r`/`localtime_s`, and shifts UTC by `TIMEZONE_OFFSET_HOURS` (+6, BST). An unrecorded datetime is the `EMPTY_DATETIME` sentinel (epoch zero), which survives save/load round-trips. Amounts are whole Taka (`unsigned int`). Enumeration values are `#define` constants; each enumeration is a `typedef unsigned char` alias, so enum-backed fields take one byte and read as their semantic type.

## Type aliases and size caps

Aliases so structs read uniformly and enum-backed fields stay one byte:

- `id_t` = `unsigned long int`
- `request_status_t` = `unsigned char` (`REQUEST_REQUESTED` / `REQUEST_APPROVED` / `REQUEST_REJECTED`)
- `staff_role_t` = `unsigned char` (`TRAINER` / `BRANCH_MANAGER`), stored in `branch_staff_t.role`
- `user_role_t` = `unsigned char` (`USER_ROLE_SYSADMIN` / `USER_ROLE_BRANCH_MANAGER` / `USER_ROLE_TRAINER` / `USER_ROLE_MEMBER`), used by `session_t`
- `membership_status_t` = `unsigned char` (`MEMBERSHIP_ON_HOLD` / `MEMBERSHIP_ACTIVE` / `MEMBERSHIP_SUSPENDED` / `MEMBERSHIP_CANCELLED`)
- `transaction_t` = `unsigned char` (`CASH_TRANSACTION` / `DIGITAL_TRANSACTION`)
- `payment_status_t` = `unsigned char` (`PAYMENT_PENDING` / `PAYMENT_COMPLETED` / `PAYMENT_FAILED` / `PAYMENT_INVALID`)

Every string field is a fixed-size `char` array capped by a macro (`FULL_NAME_BUFFER_SIZE`, `USERNAME_BUFFER_SIZE`, `PASSWORD_HASH_BUFFER_SIZE`, `EMAIL_BUFFER_SIZE`, `PHONE_BUFFER_SIZE`, `BRANCH_NAME_BUFFER_SIZE`, `REASON_BUFFER_SIZE`, `DESCRIPTION_BUFFER_SIZE`, `TRX_ID_BUFFER_SIZE`). The branch list is capped at `BRANCH_COUNT_MAX` names. No dynamic allocation is used anywhere; the input wrappers cap reads at these sizes and the file helpers sanitize against the delimiter.

## Data Model

Structs are designed as if they were database tables. Each user type is its own record type.

### User Types

- `sysadmin_t`: `id`, `username`, `password_hash` (pre-existed, no full_name/phone/email/joined_at).
- `branch_staff_t`: covers both branch managers and branch trainers in one table; `role` (`staff_role_t`: `TRAINER`, `BRANCH_MANAGER`) discriminates. Fields: `id`, `full_name`, `email`, `phone_number`, `gym_branch`, `username`, `password_hash`, `joined_at`, `role`.
- `gym_member_t`: `id`, `full_name`, `email`, `phone_number`, `gym_branch`, `username`, `password_hash`, `joined_at`, `last_payment_date`, `due_amount`, `plan` (`subscription_plan_t`), `status` (`membership_status_t`).

Membership status (`membership_status_t`): `MEMBERSHIP_ON_HOLD`, `MEMBERSHIP_ACTIVE`, `MEMBERSHIP_SUSPENDED`, `MEMBERSHIP_CANCELLED`. `MEMBERSHIP_ON_HOLD` applies only to gym members: they just self-registered and have not been approved by a branch manager yet. `MEMBERSHIP_CANCELLED` covers a member who ends their membership.

`subscription_plan_t`: `payable_amount`, `interval_days` (only applicable while a member is `MEMBERSHIP_ACTIVE`).

Usernames are globally unique across all three tables. `username_exists()` (user module) checks every table and is enforced on creation and used by login, so login is never ambiguous.

### Branches

Branches are not a table. `data/branches.txt` holds one branch name per line (array of strings, capped at `BRANCH_COUNT_MAX`). At startup, `load_branches()` reads the file into a static in-memory array; all subsequent lookups (`find_branch()`) and mutations (`add_branch()`) operate on this copy, with writes also persisted to the file. `find_branch()` is required when adding staff or gym members, so nobody is attached to a branch that does not exist. A branch's staff counts are capped by `MAX_MANAGERS_PER_BRANCH`, `MAX_TRAINERS_PER_BRANCH`, and `MAX_MEMBERS_PER_BRANCH` (defined in `settings.h`). Before creating a staff member or approving a member, the user module counts existing records of that role in the branch and rejects the operation when the cap is reached (`branch_manager_count()`, `branch_trainer_count()`, `branch_member_count()`).

### Session

`session_t` holds the logged-in user's context: `role` (`user_role_t`), `user_id`, `username`, and `branch_name` (empty for the sysadmin). It is runtime state, not a persisted record. The session module (`session.[ch]`) provides access predicates (`session_is_sysadmin()`, `session_is_staff()`, `session_belongs_to_branch()`, per-role checks) that gate every action by role and branch scope.

### Actor references

Branch managers and trainers share one table (`branch_staff_t`), so a single staff id is unambiguous across both roles and no `(role, id)` pair is needed. Members are referenced by their own id space (`gym_member_id`). The lost & found module references members by `reported_by_username` instead of an id, because staff only need the name to identify the reporter.

### Requests (approval flows)

| Table | Fields |
|---|---|
| `membership_status_change_request_t` | `request_id`, `gym_member_id`, `requested_by_staff_id`, `resolved_by_staff_id`, `reason`, `new_membership_status`, `status`, `created_at` |
| `subscription_plan_change_request_t` | `request_id`, `gym_member_id`, `new_plan`, `status`, `created_at` |
| `profile_edit_request_t` | `full_name`, `email`, `phone_number`, `gym_branch`, `username`, `status` |

- `membership_status_change_request_t`: a branch trainer requests a member's status change (approve / suspend / unsuspend / cancel). `new_membership_status` is the target status; `reason` is mandatory for suspend/unsuspend. Only a branch manager resolves it (`resolved_by_staff_id` = manager id); on approval the member's status is set (a suspension also writes a `suspension_record_t`).
- `subscription_plan_change_request_t`: a member requests switching to a new plan (`new_plan`). Branch staff of the member's branch approve or reject; on approval the member's `plan` is replaced. No resolver id is recorded, the acting staff member is implied by branch scope.
- `profile_edit_request_t`: a member requests profile changes; the fields hold the desired new values. Branch staff of the member's branch approve or reject; on approval the values are copied into the member record.
- Common request status (`request_status_t`): `REQUEST_REQUESTED`, `REQUEST_APPROVED`, `REQUEST_REJECTED`.

### Records

| Table | Fields |
|---|---|
| `payment_t` | `id`, `amount`, `transaction_time`, `transaction_type`, `transaction_id`, `status` |
| `payment_record_t` | `gym_member_id`, `payment_id` |
| `suspension_record_t` | `id`, `gym_member_id`, `reason`, `suspension_date`, `unsuspension_date` |
| `lost_and_found_record_t` | `id`, `description`, `reported_by_username`, `reported_at`, `resolved_by_staff_id` |

- `payment_t`: transaction history. `transaction_type` is `CASH_TRANSACTION` / `DIGITAL_TRANSACTION`; `status` (`payment_status_t`) is `PAYMENT_PENDING`, `PAYMENT_COMPLETED`, `PAYMENT_FAILED`, `PAYMENT_INVALID`. Cash payments are recorded as `PAYMENT_COMPLETED` by a trainer on handover; digital payments are recorded by the member and carry the status the gateway reports. Only `PAYMENT_COMPLETED` payments affect the member's account. `transaction_id` is the external reference.
- `payment_record_t`: join table linking a member to their payments (a member can have many payments; a payment belongs to exactly one member).
- `suspension_record_t`: one record per suspension event. `unsuspension_date` is nullable (`0` means still suspended).
- `lost_and_found_record_t`: `resolved_by_staff_id` is nullable (`0` means open). Resolution is signaled by a non-zero staff id, so no separate status/type/title fields are needed.

### Carriers and view models (not persisted)

- `digital_payment_request_t` (`request_id`, `gym_member_id`, `amount`, `transaction_time`, `transaction_id`, `status`): a function-argument carrier used when a member records a digital payment; a `payment_t` + `payment_record_t` are persisted from it. It is not a stored approval flow.
- `gym_member_profile_t` (`id`, `full_name`, `email`, `phone_number`, `gym_branch`, `username`, `joined_at`, `plan`): a read-only projection of a member's profile (no credentials, dues, or status), used when viewing a profile.

### Data files

`data/branches.txt`, `data/sysadmins.dat`, `data/branch_staff.dat`, `data/gym_members.dat`, `data/payments.dat`, `data/payment_records.dat`, `data/membership_status_change_requests.dat`, `data/subscription_plan_change_requests.dat`, `data/profile_edit_requests.dat`, `data/suspensions.dat`, `data/lost_and_found.dat`.

## Rules & Flows

- Members self-register from the login menu (choose an existing branch, set username/password/name/phone/email) and are created as `MEMBERSHIP_ON_HOLD`. Only the sysadmin can create staff (choosing a role) or branches.
- A branch manager approves a `MEMBERSHIP_ON_HOLD` member to `MEMBERSHIP_ACTIVE` (directly, or by resolving a trainer's status-change request). Approval assigns the default plan, sets `last_payment_date` = approval date and `due_amount` = plan `payable_amount`; the member is payable again at `last_payment_date + interval_days`.
- Digital payments are recorded by the member directly; cash payments are handed to a branch trainer who records them directly. Neither flow needs an approval request.
- Only `PAYMENT_COMPLETED` payments reduce `due_amount` (clamped at 0) and push `last_payment_date` forward to the payment date. The next due date is always `last_payment_date + plan.interval_days`.
- Auto-suspend: at startup, `MEMBERSHIP_ACTIVE` members whose due date (`last_payment_date + interval_days`) is more than `MAX_UNPAID_DAYS` (90) in the past are suspended with reason "Auto: unpaid dues". Each suspension creates a `suspension_record_t` with its reason and date.
- Only branch managers suspend or unsuspend members directly or resolve trainer status-change requests. Trainers can only request. Every suspension carries a mandatory `reason`. The sysadmin can perform these operations too.
- Members request plan changes and profile edits; branch staff approve or reject them. Staff and sysadmin edit their own records directly.
- Per-branch capacity caps: `MAX_MANAGERS_PER_BRANCH` (default 1), `MAX_TRAINERS_PER_BRANCH` (default 5), `MAX_MEMBERS_PER_BRANCH` (default 100); enforced by `branch_manager_count()`, `branch_trainer_count()`, `branch_member_count()` in the user module. Derived global caps (`MAX_BRANCH_MANAGERS`, `MAX_TRAINERS`, `MAX_GYM_MEMBERS`) equal `BRANCH_COUNT_MAX` times the respective per-branch limit and bound the static arrays that store all records.
- Suspended members can still log in, but get a banner with the reason and are restricted to paying dues, viewing history, and reporting lost/found.
- Seeded sysadmin: username `admin`, password `admin` (created on first run when no sysadmin exists).
- Member economics macros: `DEFAULT_PLAN_AMOUNT` (default plan payable amount, whole Taka), `DEFAULT_PLAN_INTERVAL_DAYS` (default plan interval in days), `MAX_UNPAID_DAYS` (grace period, 90).

## Access Control

The system administrator can perform every operation. Branch staff are limited to their own branch (managers additionally resolve status-change requests); gym members are limited to their own data.

| Action | Sysadmin | Manager | Trainer | Member |
|---|---|---|---|---|
| Create new branches | yes | no | no | no |
| Create staff (manager / trainer) | yes | no | no | no |
| Self-register as member | no | no | no | yes (MEMBERSHIP_ON_HOLD) |
| Record digital payment | yes | no | no | yes |
| Record cash payment (handover) | yes | yes | yes | no |
| View payments | all | own branch | own branch | own only |
| Approve member (MEMBERSHIP_ON_HOLD to MEMBERSHIP_ACTIVE) | yes | yes | no | no |
| Request member status change | no (acts directly) | no | yes | no |
| Resolve status-change requests | yes | yes | no | no |
| Suspend/unsuspend member directly | yes | yes | no | no |
| Request plan change | no (acts directly) | no | no | yes |
| Resolve plan-change requests | yes | yes | yes | no |
| Request profile edit | no | no | no | yes |
| Resolve profile-edit requests | yes | yes | yes | no |
| Report lost/found item | yes | no | no | yes |
| View + resolve lost/found | all | own branch | own branch | no |

Managers and trainers see only their own branch's resources; the sysadmin sees and controls everything.

## Test Strategy

Precise assert-based tests, one file per unit, run via `build/test_runner`:

- `hash`: known polynomial hash vectors, salt generation, mix_salt correctness.
- `string_util` / `file_util`: trim, split, sanitize_field, roundtrip read/write.
- `datetime_utils`: datetime_t <-> epoch seconds roundtrip against known vectors, leap years, month-end arithmetic (add_months), day arithmetic (add_days), formatting/parsing "yyyy-mm-dd hh:mm:ss".
- `branch`: add/list names, existence validation.
- `user`: sysadmin/staff/member create/get, credential helpers, username_exists() across all three tables, branch_manager_count() / branch_trainer_count() / branch_member_count() (per-branch capacity enforcement).
- `payment`: digital recorded by member, cash recorded by trainer, status handling (only PAYMENT_COMPLETED applies), due amount clamp, last_payment_date update.
- `member`: self-registration -> MEMBERSHIP_ON_HOLD, MEMBERSHIP_ON_HOLD to MEMBERSHIP_ACTIVE approval (plan assignment, due amount, last_payment_date), suspension records with an open `EMPTY_DATETIME` unsuspension date, auto-suspend sweep against the real clock (tests bracket stamped times between before/after reads).
- `request`: status-change (trainer -> manager), plan-change, and profile-edit flows.
- `lost_found`: report by username, resolve by staff id.

Tests use a temp data directory set via the `DEFAULT_DATA_DIRECTORY` macro, so they never touch real data.

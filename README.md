# Gymtrac

`Gymtrac` is a gym management system, a `group project` for the `CSE115L` course at North South University. Expecting fancy engineering? Wrong repo. Written in C against C11 standards, strictly CLI, no GUI.

> Why am I putting this on GitHub?

This is not the type of project I want to put in my account. But, the entire project is built using AI (OpenCode, model: Big Pickle) and I thought it would be fun to if I share the thing out. Needless to say, I didn't edit any single line of code, for every changes, I did write a prompt. However, this readme and AGENTS.md is human written btw.


## Disclaimer about stupidity

> This is a kind of demo project. So, it does not care about engineering!

CSE115L is the very first major CSE course, the group project is to practice the concepts taught in class. Don't expect prod-grade code. A deliberately stupid solution that works beats a heavily engineered one here. If you're looking for a repo to learn from, your rocket engine failed in a very bad way that it make you land here.

> Password hashing here stops a curious classmate, not an attacker with a GPU.

Passwords run through a polynomial hash function (similar to Java's `String.hashCode()`). This is **not** a cryptographically secure hash. Do **not** put it anywhere near production. CSE115L constraints leave no choice: dynamic memory allocation, bitwise operations, and proper hashing libraries (SHA-256, bcrypt) are all banned. Why? Because those aren't covered in this F-ing course! So this ships.

> The PRNG is xoshiro128**, great for dice rolls, useless against attackers.

Random numbers come from the [xoshiro128**](https://prng.di.unimi.it/xoshiro128starstar.c) PRNG by David Blackman and Sebastiano Vigna. Not cryptographically secure, and it must stay out of production. It's a higher-quality stand-in for standard library `rand()`. The implementation dodges bitwise operators with equivalent arithmetic. Why this was implemented? Broh, we care enough about our grades.

BTW, Reference for xoshiro128 if you care enough to learn it: <https://prng.di.unimi.it/>

> No malloc, no database, just fixed arrays with ceilings written in settings.h.

Dynamic memory allocation (`malloc`/`realloc`/`free`) is prohibited. No exceptions. Every record table lives in a static fixed-size array sized at compile time. The `MAX_*_RECORDS` macros in `settings.h` set those sizes: derived caps like `MAX_GYM_MEMBERS` bound the user tables, and event-table caps like `MAX_SUSPENSION_RECORDS` or `MAX_PAYMENT_RECORDS` bound records that accumulate over a member's lifetime. A table at its cap rejects new records outright. Nothing gets silently dropped.

> split() (string_util.c:34) knows one buffer width. Don't blame me if the compiler yells at you.

`split()` (string_util.c:34) takes its output destination as `char parts[][FIELD_BUFFER_SIZE]`, not the looser `char *parts[]`. That's deliberate. The `FIELD_BUFFER_SIZE` macro in `settings.h` controls the field width, and since dynamic allocation is off the table anyway, every caller already declares fixed `parts[][FIELD_BUFFER_SIZE]` buffers. Handing over the real 2D array kills the pointer-map boilerplate callers used to need. It also makes the compiler yell at any buffer whose row width differs from `FIELD_BUFFER_SIZE`, instead of letting a caller misreport capacities and overflow rows silently. Trade-off: `split()` only splits into buffers of that one width, so don't reach for it with arbitrary-sized field buffers.

> `static inline` saves a call, not the world.

Small helpers are `static inline` in their own `.c` file. We're not paying for calling small and plain static helper functions if it isn't inlined. Although static inline saves a call, not your broken rocket engine.

> Auto-suspend only fires when a menu action runs. Don't expect cron.

There's no background scheduler here. A member only gets suspended the next time someone hits a menu, or the next time the app boots. Sit idle on the login screen and a stale member keeps their stale status.

## Project brief

- Login takes `username` + password; passwords are stored as salted hashes. No password match, no entry.
- 3 user record types exist: System Administrator (pre-existed), Branch Staff (a `role` field distinguishes Branch Manager from Branch Trainer), and Gym Member.
- Branches are a plain list of branch names. Every staff member and gym member belongs to exactly one existing branch. One branch each, no exceptions.
- Each branch's staff and member counts are capped by `MAX_MANAGERS_PER_BRANCH`, `MAX_TRAINERS_PER_BRANCH`, and `MAX_MEMBERS_PER_BRANCH` macros defined in `settings.h`. Derived global caps (`MAX_BRANCH_MANAGERS`, `MAX_TRAINERS`, `MAX_GYM_MEMBERS`) bound the static arrays that store all records.
- A newly self-registered gym member sits at `on_hold` until a branch manager moves them to `active`. There is no other way in.
- Each member subscribes to a plan (`payable_amount` + `interval_days`). The plan sets the fee amount and the payment interval. Nothing else does.
- Members record digital payments directly. Cash goes through a branch trainer, who records it. No approval request in either path.
- Members view their own payment history.
- A completed payment reduces `due_amount` (clamped at 0) and updates `last_payment_date`; the next due date is `last_payment_date + interval_days`.
- Dues left unpaid past the due date plus a grace period trigger automatic suspension. A manager unsuspends after the dues are paid. No manager, no unsuspension.
- Members view their own profiles.
- `Lost & Found`: members report lost or found items. Branch staff view them and mark them resolved.
- Suspensions:
  - Only branch managers approve a member (`on_hold` to `active`), suspend, or unsuspend directly. Trainers hold none of this authority.
  - Every suspension carries a mandatory `reason`, stored in a dedicated suspension record with its date (and an optional unsuspension date). No reason, no suspension.
- Deletion rules:
  - A branch stays undeletable while any staff or member is assigned to it (`ensure_branch_has_no_users()` (branch.c:92)).
  - Branch staff and gym members can be deleted. A member with outstanding dues is protected. The system administrator account has no delete path.
- Access control:
  - Members see their own data. Only their own data.
  - Branch managers and trainers see only their own branch's resources (members, payments, requests, lost & found). Nothing outside their branch.
  - The system administrator can perform every operation.
  - New branches come from exactly one place: the system administrator.
  - The system administrator creates users of any type (branch managers included). Gym members self-register and stay `on_hold` until their branch manager approves them.


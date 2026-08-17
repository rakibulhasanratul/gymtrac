Hello, I am Ratul. I think it is worth to introduce myself. I am an independent developer who likes to explore different technologies and frameworks. I like to develop things in as cleanly as possible. I focus on writing maintainable and efficient code. Security matters more than anything else to me. Stupid but working solutions are often the best solutions rather than clever solutions that has potential vulnerabilities.

## About The Project
The name of the project is `Gymtrac` which is a gym management system, a `group project` for `CSE115L` course of North South University. The project is developed using C following C11 standards. This is a complete `CLI only` project, with no GUI.

### Requirements for The Codebase
- NO use of SQL or non-SQL databases. For persistence, the project should use `FILE *` macro.
- Use fgets() for string input and scanf() for the rest.
- Use of `pthread.h` is strictly prohibited.
- Passwords should be stored in hash.
- Modularity is preferred, although not a hard requirement. If modularity is applied, each module should serve only one purpose.
- The project should be implemented in `functional programming` manner.
- The project should ONLY UTILIZE standard C libraries (.h). The use is LIMITED to following headers: `stdio.h`, `string.h`, `math.h`, `time.h`, `stdbool.h`, `limits.h`, `ctype.h`.
- Variables, struct fields, parameters, and return values use only primitive data types (`int`, `char`, `float`, `double`, and their `signed`/`unsigned`/`short`/`long` variants). Fixed-width types from `stdint.h` are prohibited. Structs and typedef aliases to primitive types are allowed for record modeling; `time_t` is permitted for time fields.
- Tests are fine but those should be extremely precise, not bloated.

## Coding Standards
- Use `snake_case` names for variables, functions, and type aliases.
- Use typedef with a `_t` suffix for struct types.
- Represent enumeration values as `#define` constants and give each enumeration a `typedef unsigned char` alias, so enum-backed fields take one byte and keep a self-describing type.
- Use `UPPERCASE` for macros.
- DO NOT use constants, use macros instead.
- Hex escapes in string literals and hex numeric constants are prohibited; use plain characters and decimal literals only. The persisted-record field delimiter is `|` (pipe); field values never contain `|`.
- Use self describing names for variables, functions, and types. Avoid using single letter names or abbreviations.
- Strings are fixed-size `char` arrays with macro caps; dynamic memory allocation (`malloc`/`free`) is not used.
- The code should follow UNIX Coding Standards. Use `4 spaces` for indentation, no tabs. Use `\n` for new lines, not `\r\n`. Use `\t` for tabulation. Each function should have only one purpose.
- Use guard clauses and early returns to avoid deep nesting of code.
- Use of `switch-case` is preferred over `if-else` statements, when possible.
- Focus on writing maintainable and efficient code. Avoid writing clever code that is hard to understand or has potential vulnerabilities.
- Use of `goto` and `label` is strictly prohibited.
- If a function takes struct as an argument and it is not being modified, add `_payload` suffix to the argument name.
- Use of `global variables` is highly discouraged, although not a hard requirement.
- Use short and self explanatory comments for each typedefs and statements. Avoid writing comments for trivial code. Use comments to explain the purpose of a struct, or typedef, not how it works. Use docstring comments for functions, explaining the purpose of the function, its parameters, and return values. Use it to explain the purpose of a function, not how it works.
- Use _2 spaces_ for indentation in comments, not 4 spaces.
- Prefer arrays over pointers as variables and function parameters. Use pointer only when there is no other option or using pointers can increase readability or make it easier to comprehend to new C learners. Use array indices syntax for iterating over arrays.
- Use `()` instead of `(void)` for functions with no parameters.
- Do not introduce new blocks for single statements in `if`, `else`, `for`, and `while` statements. Use `{}` only when there are multiple statements in the block.

## Directory Structure
```
src/
├── main.c              Startup, login dispatch, session bootstrap
├── modules/
│   ├── config.[ch]     data_dir + path helpers + ensure data dir exists
│   ├── branch.[ch]     branch name list + existence validation
│   ├── user.[ch]       user record CRUD (sysadmin / branch_staff / gym_member tables), id allocation, username_exists(), branch_has_manager()
│   ├── auth.[ch]       login/logout, username + password verification
│   ├── member.[ch]     member lifecycle: approval, suspensions, auto-suspend sweep, status-change / plan-change / profile-edit requests
│   ├── payment.[ch]    payments (digital recorded by member, cash recorded by trainer) + due_amount / last_payment_date updates
│   ├── lost_found.[ch] report lost/found items, mark resolved
│   ├── session.[ch]    session_t (role, user_id, username, branch_name) + access predicates
│   └── menu.[ch]       login/register menus + one menu handler per role
└── utils/
    ├── string_util.[ch] trim, split, parse numbers, case helpers
    ├── file_util.[ch]   read/write lines, field sanitization
    ├── input.[ch]       safe fgets/scanf wrappers with validation
    ├── hash.[ch]        polynomial hash (demo) + salt generation + mix_salt helpers
    └── date_util.[ch]   time_t <-> yyyy-mm-dd conversion (day-normalized) + add_months()
```
The main.c should include modules as `#include "modules/module_name.h"` and utils as `#include "utils/util_name.h"`. The test_main.c should include modules as `#include "../src/modules/module_name.h"` and utils as `#include "../src/utils/util_name.h"`.

## Build & Run
The project is compiled with `gcc` directly, `make` is NOT used. Scripts compile and then run through the host terminal.
- `./run.sh` compiles the main program into `build/gymtrac` and runs it.
- `./run.sh test` compiles the test runner into `build/test_runner` and runs it.
- Windows users use `run.bat` and `run.bat test`.

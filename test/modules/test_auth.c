#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "../../src/modules/auth.h"
#include "../../src/modules/session.h"
#include "../../src/modules/user.h"
#include "../../src/settings.h"
#include "../../src/types.h"
#include "test_auth.h"

/**
 * Verifies that hash_password produces a valid stored format:
 * 15-char salt prefix followed by decimal hash digits.
 */
void test_hash_password_valid_format()
{
  char stored[PASSWORD_HASH_BUFFER_SIZE];

  hash_password("mypassword", stored);

  assert(strlen(stored) > 0);

  // First 15 characters must be alphanumeric (the salt).
  for (int index = 0; index < SALT_BUFFER_SIZE - 1; index++) assert(isalnum((unsigned char)stored[index]));

  // Remaining characters must be decimal digits (the hash string).
  for (int index = SALT_BUFFER_SIZE - 1; stored[index] != '\0'; index++) assert(isdigit((unsigned char)stored[index]));
}

/**
 * Verifies that hash_password produces different outputs for the same
 * password due to random salt generation.
 */
void test_hash_password_unique_salts()
{
  char stored1[PASSWORD_HASH_BUFFER_SIZE];
  char stored2[PASSWORD_HASH_BUFFER_SIZE];

  hash_password("mypassword", stored1);
  hash_password("mypassword", stored2);

  assert(strcmp(stored1, stored2) != 0);
}

/**
 * Verifies that verify_password accepts the correct password.
 */
void test_verify_password_correct()
{
  char stored[PASSWORD_HASH_BUFFER_SIZE];

  hash_password("secret123", stored);
  assert(verify_password("secret123", stored) == true);
}

/**
 * Verifies that verify_password rejects a wrong password.
 */
void test_verify_password_wrong()
{
  char stored[PASSWORD_HASH_BUFFER_SIZE];

  hash_password("secret123", stored);
  assert(verify_password("wrongpass", stored) == false);
}

/**
 * Verifies that verify_password rejects an empty password against a
 * non-empty stored hash.
 */
void test_verify_password_empty_vs_nonempty()
{
  char stored[PASSWORD_HASH_BUFFER_SIZE];

  hash_password("secret123", stored);
  assert(verify_password("", stored) == false);
}

/**
 * Verifies that verify_password handles empty password stored hash.
 */
void test_verify_password_empty_password()
{
  char stored[PASSWORD_HASH_BUFFER_SIZE];

  hash_password("", stored);
  assert(verify_password("", stored) == true);
  assert(verify_password("notempty", stored) == false);
}

/**
 * Verifies that verify_password returns false for NULL inputs.
 */
void test_verify_password_null_returns_false()
{
  char stored[PASSWORD_HASH_BUFFER_SIZE];

  hash_password("test", stored);
  assert(verify_password(NULL, stored) == false);
  assert(verify_password("test", NULL) == false);
  assert(verify_password(NULL, NULL) == false);
}

/**
 * Verifies that hash_password is safe with NULL inputs.
 */
void test_hash_password_null_is_safe()
{
  char buffer[PASSWORD_HASH_BUFFER_SIZE];

  hash_password(NULL, buffer);
  hash_password("test", NULL);
  hash_password(NULL, NULL);
}

// ---- auth_login / auth_logout tests ----

static void cleanup_auth_test_files()
{
  remove(SYSADMINS_FILE_PATH);
  remove(BRANCH_STAFF_FILE_PATH);
  remove(GYM_MEMBERS_FILE_PATH);
}

/**
 * Verifies that auth_login succeeds with correct sysadmin credentials
 * and populates the session.
 */
void test_auth_login_sysadmin_success()
{
  cleanup_auth_test_files();
  load_sysadmins();
  load_branch_staff();
  load_gym_members();
  session_init();

  char stored[PASSWORD_HASH_BUFFER_SIZE];
  hash_password("adminpass", stored);
  create_sysadmin("admin", stored);

  user_role_t role;
  bool result = auth_login("admin", "adminpass", &role);

  assert(result == true);
  assert(role == USER_ROLE_SYSADMIN);
  assert(session_is_active() == true);
  assert(session_is_sysadmin() == true);
}

/**
 * Verifies that auth_login succeeds with correct branch manager credentials.
 */
void test_auth_login_branch_manager_success()
{
  cleanup_auth_test_files();
  load_sysadmins();
  load_branch_staff();
  load_gym_members();
  session_init();

  char stored[PASSWORD_HASH_BUFFER_SIZE];
  hash_password("mgrpass", stored);
  create_branch_staff("Manager One", "m1@test.com", "0171111111", "Dhanmondi", "mgr1", stored, BRANCH_MANAGER);

  user_role_t role;
  bool result = auth_login("mgr1", "mgrpass", &role);

  assert(result == true);
  assert(role == USER_ROLE_BRANCH_MANAGER);
  assert(session_is_branch_manager() == true);
  assert(session_belongs_to_branch("Dhanmondi") == true);
}

/**
 * Verifies that auth_login succeeds with correct trainer credentials.
 */
void test_auth_login_trainer_success()
{
  cleanup_auth_test_files();
  load_sysadmins();
  load_branch_staff();
  load_gym_members();
  session_init();

  char stored[PASSWORD_HASH_BUFFER_SIZE];
  hash_password("trpass", stored);
  create_branch_staff("Trainer One", "t1@test.com", "0172222222", "Gulshan", "tr1", stored, TRAINER);

  user_role_t role;
  bool result = auth_login("tr1", "trpass", &role);

  assert(result == true);
  assert(role == USER_ROLE_TRAINER);
  assert(session_is_trainer() == true);
  assert(session_belongs_to_branch("Gulshan") == true);
}

/**
 * Verifies that auth_login succeeds with correct gym member credentials.
 */
void test_auth_login_member_success()
{
  cleanup_auth_test_files();
  load_sysadmins();
  load_branch_staff();
  load_gym_members();
  session_init();

  char stored[PASSWORD_HASH_BUFFER_SIZE];
  hash_password("mempass", stored);

  subscription_plan_t plan;
  plan.payable_amount = 1000;
  plan.interval_days = 30;
  create_gym_member("Member One", "m1@test.com", "0181111111", "Uttara", "mem1", stored, plan, MEMBERSHIP_ACTIVE);

  user_role_t role;
  bool result = auth_login("mem1", "mempass", &role);

  assert(result == true);
  assert(role == USER_ROLE_MEMBER);
  assert(session_is_member() == true);
  assert(session_belongs_to_branch("Uttara") == true);
}

/**
 * Verifies that auth_login rejects wrong password.
 */
void test_auth_login_wrong_password()
{
  cleanup_auth_test_files();
  load_sysadmins();
  load_branch_staff();
  load_gym_members();
  session_init();

  char stored[PASSWORD_HASH_BUFFER_SIZE];
  hash_password("correct", stored);
  create_sysadmin("admin", stored);

  user_role_t role;
  bool result = auth_login("admin", "wrong", &role);

  assert(result == false);
  assert(session_is_active() == false);
}

/**
 * Verifies that auth_login rejects non-existent username.
 */
void test_auth_login_unknown_username()
{
  cleanup_auth_test_files();
  load_sysadmins();
  load_branch_staff();
  load_gym_members();
  session_init();

  user_role_t role;
  bool result = auth_login("nobody", "pass", &role);

  assert(result == false);
  assert(session_is_active() == false);
}

/**
 * Verifies that auth_login handles NULL inputs safely.
 */
void test_auth_login_null_inputs()
{
  user_role_t role;
  assert(auth_login(NULL, "pass", &role) == false);
  assert(auth_login("user", NULL, &role) == false);
  assert(auth_login("user", "pass", NULL) == false);
}

/**
 * Verifies that auth_logout clears the session.
 */
void test_auth_logout_clears_session()
{
  cleanup_auth_test_files();
  load_sysadmins();
  load_branch_staff();
  load_gym_members();
  session_init();

  char stored[PASSWORD_HASH_BUFFER_SIZE];
  hash_password("pass", stored);
  create_sysadmin("admin", stored);

  user_role_t role;
  auth_login("admin", "pass", &role);
  assert(session_is_active() == true);

  auth_logout();
  assert(session_is_active() == false);
  assert(session_get_current() == NULL);
}

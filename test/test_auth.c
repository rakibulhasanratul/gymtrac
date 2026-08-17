#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "../src/modules/auth.h"
#include "../src/settings.h"
#include "test_auth.h"

/**
 * Verifies that hash_password produces a valid stored format:
 * 15-char salt prefix followed by decimal hash digits.
 */
static void test_hash_password_valid_format()
{
  char stored[PASSWORD_HASH_BUFFER_SIZE];

  hash_password("mypassword", stored);

  assert(strlen(stored) > 0);

  // First 15 characters must be alphanumeric (the salt).
  for (int index = 0; index < SALT_BUFFER_SIZE - 1; index++)
  {
    char ch = stored[index];
    bool is_upper = (ch >= 'A' && ch <= 'Z');
    bool is_lower = (ch >= 'a' && ch <= 'z');
    bool is_digit = (ch >= '0' && ch <= '9');
    assert(is_upper || is_lower || is_digit);
  }

  // Remaining characters must be decimal digits (the hash string).
  for (int index = SALT_BUFFER_SIZE - 1; stored[index] != '\0'; index++)
    assert(stored[index] >= '0' && stored[index] <= '9');
}

/**
 * Verifies that hash_password produces different outputs for the same
 * password due to random salt generation.
 */
static void test_hash_password_unique_salts()
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
static void test_verify_password_correct()
{
  char stored[PASSWORD_HASH_BUFFER_SIZE];

  hash_password("secret123", stored);
  assert(verify_password("secret123", stored) == true);
}

/**
 * Verifies that verify_password rejects a wrong password.
 */
static void test_verify_password_wrong()
{
  char stored[PASSWORD_HASH_BUFFER_SIZE];

  hash_password("secret123", stored);
  assert(verify_password("wrongpass", stored) == false);
}

/**
 * Verifies that verify_password rejects an empty password against a
 * non-empty stored hash.
 */
static void test_verify_password_empty_vs_nonempty()
{
  char stored[PASSWORD_HASH_BUFFER_SIZE];

  hash_password("secret123", stored);
  assert(verify_password("", stored) == false);
}

/**
 * Verifies that verify_password handles empty password stored hash.
 */
static void test_verify_password_empty_password()
{
  char stored[PASSWORD_HASH_BUFFER_SIZE];

  hash_password("", stored);
  assert(verify_password("", stored) == true);
  assert(verify_password("notempty", stored) == false);
}

/**
 * Verifies that verify_password returns false for NULL inputs.
 */
static void test_verify_password_null_returns_false()
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
static void test_hash_password_null_is_safe()
{
  char buffer[PASSWORD_HASH_BUFFER_SIZE];

  hash_password(NULL, buffer);
  hash_password("test", NULL);
  hash_password(NULL, NULL);
}

void run_all_auth_tests()
{
  test_hash_password_valid_format();
  test_hash_password_unique_salts();
  test_verify_password_correct();
  test_verify_password_wrong();
  test_verify_password_empty_vs_nonempty();
  test_verify_password_empty_password();
  test_verify_password_null_returns_false();
  test_hash_password_null_is_safe();
}

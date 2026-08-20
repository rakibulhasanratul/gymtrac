#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "../../src/modules/auth.h"
#include "../../src/settings.h"
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
  for (int index = 0; index < SALT_BUFFER_SIZE - 1; index++)
    assert(isalnum((unsigned char)stored[index]));

  // Remaining characters must be decimal digits (the hash string).
  for (int index = SALT_BUFFER_SIZE - 1; stored[index] != '\0'; index++)
    assert(isdigit((unsigned char)stored[index]));
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

#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "../../src/settings.h"
#include "../../src/utils/hash.h"
#include "test_hash.h"

/**
 * Verifies that create_hash matches known Java String.hashCode() vectors.
 */
void test_create_hash_known_vectors()
{
  assert(create_hash("Hello") == 69609650);
  assert(create_hash("") == 0);
  assert(create_hash("a") == 97);
  assert(create_hash("ab") == 3105);
  assert(create_hash("abc") == 96354);
}

/**
 * Verifies that create_hash returns 0 for NULL input.
 */
void test_create_hash_null_returns_zero()
{
  assert(create_hash(NULL) == 0);
}

/**
 * Verifies that generate_salt produces exactly 15 alphanumeric characters.
 */
void test_generate_salt_length_and_charset()
{
  char salt[SALT_BUFFER_SIZE];

  generate_salt(salt);

  assert((int)strlen(salt) == SALT_BUFFER_SIZE - 1);

  for (int index = 0; salt[index] != '\0'; index++) assert(isalnum((unsigned char)salt[index]));
}

/**
 * Verifies that generate_salt does not crash on NULL input.
 */
void test_generate_salt_null_is_safe()
{
  generate_salt(NULL);
}

/**
 * Verifies that mix_salt sandwiches the password between salt halves.
 */
void test_mix_salt_sandwich_output()
{
  char mixed[128];

  mix_salt("password", "ABCDEFGHIJKLMNO", mixed);
  assert(strcmp(mixed, "ABCDEFGpasswordIJKLMNO") == 0);
}

/**
 * Verifies that mix_salt handles empty password.
 */
void test_mix_salt_empty_password()
{
  char mixed[128];

  mix_salt("", "ABCDEFGHIJKLMNO", mixed);
  assert(strcmp(mixed, "ABCDEFGIJKLMNO") == 0);
}

/**
 * Verifies that mix_salt is safe with NULL inputs.
 */
void test_mix_salt_null_is_safe()
{
  char mixed[128];

  mix_salt(NULL, "ABCDEFGHIJKLMNO", mixed);
  mix_salt("password", NULL, mixed);
  mix_salt("password", "ABCDEFGHIJKLMNO", NULL);
}

/**
 * Verifies that compare_hash returns true for equal values.
 */
void test_compare_hash_equal()
{
  assert(compare_hash(69609650, 69609650) == true);
  assert(compare_hash(0, 0) == true);
  // 32-bit safe value so the test holds on both LLP64 and LP64 platforms.
  assert(compare_hash(4294967290ul, 4294967290ul) == true);
}

/**
 * Verifies that compare_hash returns false for different values.
 */
void test_compare_hash_different()
{
  assert(compare_hash(69609650, 69609651) == false);
  assert(compare_hash(0, 1) == false);
  assert(compare_hash(97, 98) == false);
}

/**
 * Verifies that hash_value_to_string produces correct decimal output.
 */
void test_hash_value_to_string_decimal()
{
  char buffer[HASH_STRING_BUFFER_SIZE];

  hash_value_to_string(69609650, buffer);
  assert(strcmp(buffer, "69609650") == 0);

  hash_value_to_string(0, buffer);
  assert(strcmp(buffer, "0") == 0);

  hash_value_to_string(97, buffer);
  assert(strcmp(buffer, "97") == 0);
}

/**
 * Verifies that hash_value_to_string is safe with NULL destination.
 */
void test_hash_value_to_string_null_is_safe()
{
  hash_value_to_string(69609650, NULL);
}

/**
 * Verifies that parse_hash_value correctly parses decimal strings.
 */
void test_parse_hash_value_decimal()
{
  assert(parse_hash_value("69609650") == 69609650);
  assert(parse_hash_value("0") == 0);
  assert(parse_hash_value("97") == 97);
  assert(parse_hash_value("4294967290") == 4294967290ul);
}

/**
 * Verifies that parse_hash_value returns 0 for invalid input.
 */
void test_parse_hash_value_invalid_returns_zero()
{
  assert(parse_hash_value("") == 0);
  assert(parse_hash_value(NULL) == 0);
  assert(parse_hash_value("abc") == 0);
  assert(parse_hash_value("12abc") == 0);
}

/**
 * Verifies that hash_value_to_string and parse_hash_value round-trip.
 */
void test_hash_value_to_string_parse_round_trip()
{
  char buffer[HASH_STRING_BUFFER_SIZE];
  hash_t original = 69609650;

  hash_value_to_string(original, buffer);
  hash_t parsed = parse_hash_value(buffer);

  assert(compare_hash(original, parsed));
}

/**
 * Verifies that a mixed string hashes consistently.
 */
void test_create_hash_consistency()
{
  char mixed1[128];
  char mixed2[128];

  mix_salt("mypassword", "ABCDEFGHIJKLMNO", mixed1);
  mix_salt("mypassword", "ABCDEFGHIJKLMNO", mixed2);

  assert(compare_hash(create_hash(mixed1), create_hash(mixed2)));
}

/**
 * Verifies that different salts produce different hashes for the same password.
 */
void test_different_salts_different_hashes()
{
  char mixed1[128];
  char mixed2[128];

  mix_salt("mypassword", "AAAAAAAAAAAAAAA", mixed1);
  mix_salt("mypassword", "BBBBBBBBBBBBBBB", mixed2);

  assert(create_hash(mixed1) != create_hash(mixed2));
}

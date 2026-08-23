// WARNING: This file implements a DEMO hashing function for educational
// purposes only. The polynomial hash used here (h = 31 * h + c) is NOT
// cryptographically secure and should NEVER be used in production.
//
// It is used in this project due to CSE115L constraints that prohibit
// dynamic memory allocation, bitwise operations, and proper hashing
// libraries (e.g., SHA-256, bcrypt).

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "../settings.h"
#include "hash.h"
#include "rng.h"
#include "string_util.h"

void generate_salt(char *destination)
{
  if (destination == NULL) return;

  // Fill 15 characters from the alphanumeric charset.
  int charset_size = (int)strlen(SALT_CHARSET);
  for (int i = 0; i < SALT_BUFFER_SIZE - 1; i++) destination[i] = SALT_CHARSET[random_number() % charset_size];
  destination[SALT_BUFFER_SIZE - 1] = '\0';
}

void mix_salt(const char *password, const char *salt, char *destination)
{
  if (password == NULL || salt == NULL || destination == NULL) return;

  int write_index = 0;

  // Copy first 7 characters of salt (indices 0-6).
  for (int i = 0; i < 7 && salt[i] != '\0'; i++) destination[write_index++] = salt[i];

  // Copy the password.
  for (int i = 0; password[i] != '\0'; i++) destination[write_index++] = password[i];

  // Copy salt characters 8 through 14 (skipping index 7).
  for (int i = 8; i < 15 && salt[i] != '\0'; i++) destination[write_index++] = salt[i];

  destination[write_index] = '\0';
}

hash_t create_hash(const char *text)
{
  if (text == NULL) return 0;

  hash_t hash_value = 0;
  for (int i = 0; text[i] != '\0'; i++) hash_value = POLYNOMIAL_MULTIPLIER * hash_value + (unsigned char)text[i];

  return hash_value;
}

bool compare_hash(hash_t stored, hash_t computed)
{
  return stored == computed;
}

void hash_value_to_string(hash_t value, char *destination)
{
  if (destination == NULL) return;

  sprintf(destination, "%lu", value);
}

hash_t parse_hash_value(const char *text)
{
  return string_to_unsigned_long_int(text);
}

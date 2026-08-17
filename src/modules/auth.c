// WARNING: This file implements password hashing using a DEMO polynomial
// hash function for educational purposes only. It is NOT cryptographically
// secure and should NEVER be used in production.
//
// Used in this project due to CSE115L constraints that prohibit dynamic
// memory allocation, bitwise operations, and proper hashing libraries.

#include <stdbool.h>
#include <string.h>

#include "../settings.h"
#include "../utils/hash.h"

// Buffer for the mixed salt+password string.
#define MIXED_BUFFER_SIZE 128

void hash_password(const char *password, char *destination)
{
  if (password == NULL || destination == NULL)
    return;

  char salt[SALT_BUFFER_SIZE];
  generate_salt(salt);

  char mixed[MIXED_BUFFER_SIZE];
  mix_salt(password, salt, mixed);

  hash_t hash_value = create_hash(mixed);

  char hash_str[HASH_STRING_BUFFER_SIZE];
  hash_value_to_string(hash_value, hash_str);

  // Store as: 15-char salt + hash decimal string.
  int write_index = 0;

  // Copy salt without null terminator.
  for (int index = 0; index < SALT_BUFFER_SIZE - 1 && salt[index] != '\0'; index++)
    destination[write_index++] = salt[index];

  // Append the hash decimal string.
  for (int index = 0; hash_str[index] != '\0'; index++)
    destination[write_index++] = hash_str[index];

  destination[write_index] = '\0';
}

bool verify_password(const char *password, const char *stored_hash)
{
  if (password == NULL || stored_hash == NULL)
    return false;

  // Extract the first 15 characters as the salt.
  char salt[SALT_BUFFER_SIZE];
  int index = 0;
  for (; index < SALT_BUFFER_SIZE - 1 && stored_hash[index] != '\0'; index++)
    salt[index] = stored_hash[index];
  salt[index] = '\0';

  // Mix the input password with the extracted salt.
  char mixed[MIXED_BUFFER_SIZE];
  mix_salt(password, salt, mixed);

  // Compute hash and compare with the stored hash.
  hash_t computed = create_hash(mixed);
  hash_t stored = parse_hash_value(stored_hash + SALT_BUFFER_SIZE - 1);

  return compare_hash(stored, computed);
}

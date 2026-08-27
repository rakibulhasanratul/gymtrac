// WARNING: DEMO polynomial password hashing for educational purposes only;
// NOT cryptographically secure, never use it in production.
//
// CSE115L bans dynamic memory allocation, bitwise operations, and proper
// hashing libraries, hence this approach.

#include <stdbool.h>
#include <string.h>

#include "../settings.h"
#include "../types.h"
#include "../utils/hash.h"
#include "session.h"
#include "user.h"

// Buffer for the mixed salt+password string.
#define MIXED_BUFFER_SIZE 128

void hash_password(const char password[], char *destination)
{
  if (password == NULL || destination == NULL) return;

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
  for (int i = 0; i < SALT_BUFFER_SIZE - 1 && salt[i] != '\0'; i++) destination[write_index++] = salt[i];

  // Append the hash decimal string.
  for (int i = 0; hash_str[i] != '\0'; i++) destination[write_index++] = hash_str[i];

  destination[write_index] = '\0';
}

bool verify_password(const char password[], const char stored_hash[])
{
  if (password == NULL || stored_hash == NULL) return false;

  // Extract the first 15 characters as the salt.
  char salt[SALT_BUFFER_SIZE];
  int i = 0;
  for (; i < SALT_BUFFER_SIZE - 1 && stored_hash[i] != '\0'; i++) salt[i] = stored_hash[i];
  salt[i] = '\0';

  // Mix the input password with the extracted salt.
  char mixed[MIXED_BUFFER_SIZE];
  mix_salt(password, salt, mixed);

  // Compute hash and compare with the stored hash.
  hash_t computed = create_hash(mixed);
  hash_t stored = parse_hash_value(stored_hash + SALT_BUFFER_SIZE - 1);

  return compare_hash(stored, computed);
}

bool auth_login(const char username[], const char password[], user_role_t *destination)
{
  if (username == NULL || password == NULL || destination == NULL) return false;

  // Check sysadmin table.
  sysadmin_t sysadmin;
  if (get_sysadmin_by_username(username, &sysadmin))
  {
    if (!verify_password(password, sysadmin.password_hash)) return false;

    *destination = USER_ROLE_SYSADMIN;
    set_session_context(USER_ROLE_SYSADMIN, sysadmin.id, sysadmin.username, "");
    return true;
  }

  // Check branch staff table.
  branch_staff_t staff;
  if (get_branch_staff_by_username(username, &staff))
  {
    if (!verify_password(password, staff.password_hash)) return false;

    switch (staff.role)
    {
    case BRANCH_MANAGER:
      *destination = USER_ROLE_BRANCH_MANAGER;
      break;
    default:
      *destination = USER_ROLE_TRAINER;
      break;
    }

    set_session_context(*destination, staff.id, staff.username, staff.gym_branch);
    return true;
  }

  // Check gym member table.
  gym_member_t member;
  if (get_gym_member_by_username(username, &member))
  {
    if (!verify_password(password, member.password_hash)) return false;

    *destination = USER_ROLE_MEMBER;
    set_session_context(USER_ROLE_MEMBER, member.id, member.username, member.gym_branch);
    return true;
  }

  return false;
}

void auth_logout()
{
  clear_session_context();
}

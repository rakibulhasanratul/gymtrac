#ifndef GYMTRAC_AUTH_H
#define GYMTRAC_AUTH_H

#include <stdbool.h>

#include "../settings.h"
#include "../types.h"

/**
 * Hashes a password with a generated salt and stores the result.
 *
 * The stored format is: 15-char salt + decimal hash string.
 *
 * @param password the plaintext password to hash
 * @param destination receives the salted hash string;
 *                    must hold at least PASSWORD_HASH_BUFFER_SIZE characters
 */
void hash_password(const char *password, char *destination);

/**
 * Verifies a password against a stored salted hash.
 *
 * @param password the plaintext password to check
 * @param stored_hash the previously stored salt+hash string
 * @return true if the password matches, false otherwise
 */
bool verify_password(const char *password, const char *stored_hash);

/**
 * Authenticates a user by username and password.
 *
 * Looks up the username across all three user tables, verifies the
 * password against the stored salted hash, and populates the session
 * on success.
 *
 * @param username the login name to look up
 * @param password the plaintext password to verify
 * @param role_destination receives the user's role on success
 * @return true if authentication succeeded, false otherwise
 */
bool auth_login(const char username[], const char password[], user_role_t *role_destination);

/**
 * Logs out the current user by clearing the session.
 */
void auth_logout();

#endif

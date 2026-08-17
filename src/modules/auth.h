#ifndef GYMTRAC_AUTH_H
#define GYMTRAC_AUTH_H

#include <stdbool.h>

#include "../settings.h"

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

#endif

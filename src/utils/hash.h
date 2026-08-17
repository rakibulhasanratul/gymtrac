#ifndef GYMTRAC_HASH_H
#define GYMTRAC_HASH_H

#include <stdbool.h>

// Polynomial hash value type (demo, not cryptographically secure).
typedef unsigned long hash_t;

/**
 * Generates a 15-character random alphanumeric salt.
 *
 * @param destination receives the null-terminated salt string;
 *                    must hold at least SALT_BUFFER_SIZE characters
 */
void generate_salt(char *destination);

/**
 * Sandwiches password between two halves of the salt.
 *
 * Produces: salt[0..6] + password + salt[8..14]
 *
 * @param password the password to embed
 * @param salt the null-terminated salt string (at least 15 characters)
 * @param destination receives the mixed string
 */
void mix_salt(const char *password, const char *salt, char *destination);

/**
 * Computes a polynomial hash of text (h = 31 * h + c).
 *
 * This is a demo hash function similar to Java's String.hashCode().
 * It is NOT cryptographically secure.
 *
 * @param text the string to hash
 * @return the polynomial hash value
 */
hash_t create_hash(const char *text);

/**
 * Compares two hash values for equality.
 *
 * @param stored the stored hash value
 * @param computed the freshly computed hash value
 * @return true if both values are equal, false otherwise
 */
bool compare_hash(hash_t stored, hash_t computed);

/**
 * Converts a hash value to its decimal string representation.
 *
 * @param value the hash value to convert
 * @param destination receives the decimal string;
 *                    must hold at least HASH_STRING_BUFFER_SIZE characters
 */
void hash_value_to_string(hash_t value, char *destination);

/**
 * Parses a decimal string back into a hash value.
 *
 * @param text the decimal string to parse
 * @return the parsed hash value, or 0 if the string is invalid
 */
hash_t parse_hash_value(const char *text);

#endif

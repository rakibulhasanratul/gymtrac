#ifndef GYMTRAC_SETTINGS_H
#define GYMTRAC_SETTINGS_H

// Buffer sizes for fixed-size char array fields.
#define FULL_NAME_BUFFER_SIZE 64
#define USERNAME_BUFFER_SIZE 32
#define PASSWORD_HASH_BUFFER_SIZE 40 // salt (15 chars) + polynomial hash decimal + null
#define EMAIL_BUFFER_SIZE 64
#define PHONE_BUFFER_SIZE 11        // phone number pattern: 01XXXXXXXXX
#define BRANCH_NAME_BUFFER_SIZE 128 // should allow detailed location names
#define REASON_BUFFER_SIZE 128      // suspension/request reason field
#define DESCRIPTION_BUFFER_SIZE 256 // description field for lost and found
#define TRX_ID_BUFFER_SIZE 64       // transaction id field
#define DATE_BUFFER_SIZE 11         // yyyy-mm-dd + null terminator

// Hashing internals.
#define SALT_BUFFER_SIZE 16         // 15 printable chars + null
#define HASH_STRING_BUFFER_SIZE 32  // decimal representation of unsigned long + null
#define POLYNOMIAL_MULTIPLIER 31    // multiplier matching Java's String.hashCode()
#define SALT_CHARSET "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789"

// Capacity limits: branch count, per-branch caps, and derived global caps.
#define BRANCH_COUNT_MAX 32
#define MAX_MANAGERS_PER_BRANCH 1
#define MAX_TRAINERS_PER_BRANCH 5
#define MAX_MEMBERS_PER_BRANCH 100
#define MAX_BRANCH_MANAGERS (BRANCH_COUNT_MAX * MAX_MANAGERS_PER_BRANCH)
#define MAX_TRAINERS (BRANCH_COUNT_MAX * MAX_TRAINERS_PER_BRANCH)
#define MAX_GYM_MEMBERS (BRANCH_COUNT_MAX * MAX_MEMBERS_PER_BRANCH)

// File storage.
#define FIELD_DELIMITER '|'         // delimiter separating fields in a persisted record
#define DEFAULT_DATA_DIRECTORY "data"
#define GYM_BRANCHES_FILENAME "branches.txt"
#define PATH_BUFFER_SIZE 256        // buffer for resolved file paths

#endif

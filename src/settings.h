#ifndef GYMTRAC_SETTINGS_H
#define GYMTRAC_SETTINGS_H

#define FULL_NAME_BUFFER_SIZE 64
#define USERNAME_BUFFER_SIZE 32
#define PASSWORD_HASH_BUFFER_SIZE 40 // salt (15 chars) + polynomial hash decimal + null
#define SALT_BUFFER_SIZE 16          // 15 printable chars + null
#define HASH_STRING_BUFFER_SIZE 32   // decimal representation of unsigned long + null
#define EMAIL_BUFFER_SIZE 64
#define PHONE_BUFFER_SIZE 11        // phone number pattern: 01XXXXXXXXX
#define BRANCH_NAME_BUFFER_SIZE 128 // should allow detailed location names
#define BRANCH_COUNT_MAX 32         // maximum number of branch entries
#define REASON_BUFFER_SIZE 128      // suspension/request reason field
#define DESCRIPTION_BUFFER_SIZE 256 // description field for lost and found
#define TRX_ID_BUFFER_SIZE 64       // transaction id field
#define DATE_BUFFER_SIZE 11         // yyyy-mm-dd + null terminator
#define FIELD_DELIMITER '|'         // delimiter separating fields in a persisted record
#define POLYNOMIAL_MULTIPLIER 31    // multiplier matching Java's String.hashCode()
#define SALT_CHARSET "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789"
#define DEFAULT_DATA_DIRECTORY "data" // default directory for persisted records

#endif

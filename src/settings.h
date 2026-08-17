#ifndef GYMTRAC_SETTINGS_H
#define GYMTRAC_SETTINGS_H

#define FULL_NAME_BUFFER_SIZE 64
#define USERNAME_BUFFER_SIZE 32
#define PASSWORD_HASH_BUFFER_SIZE 130 // salted SHA-256 hash field length cap
#define EMAIL_BUFFER_SIZE 64
#define PHONE_BUFFER_SIZE 11        // phone number pattern: 01XXXXXXXXX
#define BRANCH_NAME_BUFFER_SIZE 128 // should allow detailed location names
#define BRANCH_COUNT_MAX 32         // maximum number of branch entries
#define REASON_BUFFER_SIZE 128      // suspension/request reason field
#define DESCRIPTION_BUFFER_SIZE 256 // description field for lost and found
#define TRX_ID_BUFFER_SIZE 64       // transaction id field
#define FIELD_DELIMITER '|'         // delimiter separating fields in a persisted record

#endif

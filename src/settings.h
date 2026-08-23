#ifndef GYMTRAC_SETTINGS_H
#define GYMTRAC_SETTINGS_H

// Buffer sizes for fixed-size char array fields.
#define FULL_NAME_BUFFER_SIZE 64
#define USERNAME_BUFFER_SIZE 32
#define PASSWORD_HASH_BUFFER_SIZE 40 // salt (15 chars) + polynomial hash decimal + null
#define EMAIL_BUFFER_SIZE 64
#define PHONE_BUFFER_SIZE 11        // phone number pattern: 01XXXXXXXXX
#define BRANCH_NAME_BUFFER_SIZE 128 // should allow detailed location names
#define REASON_BUFFER_SIZE 1024     // suspension/request reason field
#define DESCRIPTION_BUFFER_SIZE 256 // description field for lost and found
#define TRX_ID_BUFFER_SIZE 64       // transaction id field
#define DATE_BUFFER_SIZE 11         // yyyy-mm-dd + null terminator
#define FIELD_BUFFER_SIZE 256       // buffer for a single field when splitting a pipe-delimited record line
#define LINE_BUFFER_SIZE 1024       // one full record line, large enough for any record

// Hashing internals.
#define SALT_BUFFER_SIZE 16        // 15 printable chars + null
#define HASH_STRING_BUFFER_SIZE 32 // decimal representation of unsigned long + null
#define POLYNOMIAL_MULTIPLIER 31   // multiplier matching Java's String.hashCode()
#define SALT_CHARSET "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789"

// Capacity limits: branch count, per-branch caps, and derived global caps.
#define BRANCH_COUNT_MAX 32
#define MAX_MANAGERS_PER_BRANCH 1
#define MAX_TRAINERS_PER_BRANCH 5
#define MAX_MEMBERS_PER_BRANCH 100
#define MAX_SYSTEM_ADMINS 1
#define MAX_BRANCH_MANAGERS (BRANCH_COUNT_MAX * MAX_MANAGERS_PER_BRANCH)
#define MAX_TRAINERS (BRANCH_COUNT_MAX * MAX_TRAINERS_PER_BRANCH)
#define MAX_GYM_MEMBERS (BRANCH_COUNT_MAX * MAX_MEMBERS_PER_BRANCH)
#define MAX_SUSPENSION_RECORDS (MAX_GYM_MEMBERS * 2) // a member may be suspended more than once

// Member economics: default plan assigned on approval and dues grace period.
#define DEFAULT_PLAN_AMOUNT 1000      // default plan payable amount in whole Taka
#define DEFAULT_PLAN_INTERVAL_DAYS 30 // default plan payment interval in days
#define MAX_UNPAID_DAYS 90            // days past the due date before auto-suspension
#define AUTO_SUSPENSION_REASON "Auto: unpaid dues"

// File storage.
#ifndef DATA_DIRECTORY
#define DATA_DIRECTORY "data"
#endif
// the delimiter needs to be verified to be 1 character long in main function
#define FIELD_DELIMITER_STRING "|"
#define FIELD_DELIMITER FIELD_DELIMITER_STRING[0]
#define GYM_BRANCHES_FILE_PATH DATA_DIRECTORY "/branches.txt"
#define SYSADMINS_FILE_PATH DATA_DIRECTORY "/sysadmins.dat"
#define BRANCH_STAFF_FILE_PATH DATA_DIRECTORY "/branch_staff.dat"
#define GYM_MEMBERS_FILE_PATH DATA_DIRECTORY "/gym_members.dat"
#define SUSPENSIONS_FILE_PATH DATA_DIRECTORY "/suspensions.dat"
#define MAX_RECORD_FIELDS 14 // maximum number of fields in any pipe-delimited record

// Credentials for the sysadmin account seeded on first run.
#define DEFAULT_SYSADMIN_USERNAME "admin"
#define DEFAULT_SYSADMIN_PASSWORD "admin123"

// Error logging macro. Prints to stderr with source location.
// Wrapped in do-while(0) so it behaves as a single statement in if/else blocks.
#define LOG_ERROR(msg, ...)                                                                                            \
  do                                                                                                                   \
  {                                                                                                                    \
    fprintf(stderr, "%s:%d: " msg "\n", __FILE__, __LINE__, ##__VA_ARGS__);                                            \
  } while (0)

// windows compatibility layer
#if defined(_WIN32) || defined(_WIN64)
#define localtime_r(timep, result) (localtime_s(result, timep) == 0 ? result : NULL)
#endif

#endif

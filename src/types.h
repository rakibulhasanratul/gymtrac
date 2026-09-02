#ifndef GYMTRAC_TYPES_H
#define GYMTRAC_TYPES_H

#include "settings.h"

// Common numeric id type shared by every record.
typedef unsigned long int id_t;

// Calendar timestamp with second precision.
typedef struct
{
  int year;   // full year, e.g. 2026; must be at least EPOCH_YEAR
  int month;  // 1-12
  int day;    // 1-31, valid for the month
  int hour;   // 0-23
  int minute; // 0-59
  int second; // 0-59
} datetime_t;

// Datetime meaning "nothing recorded yet"; an open suspension uses it until
// unsuspension. Matches epoch 0, the value an unrecorded datetime stores in
// data files, so the sentinel survives save/load round-trips.
#define EMPTY_DATETIME ((datetime_t){1970, 1, 1, 0, 0, 0})

// System administrator record, seeded on first run.
typedef struct
{
  id_t id;
  char username[USERNAME_BUFFER_SIZE];
  char password_hash[PASSWORD_HASH_BUFFER_SIZE];
} sysadmin_t;

// Staff role distinguishing a branch trainer from a branch manager.
typedef unsigned char staff_role_t;
#define TRAINER 0
#define BRANCH_MANAGER 1

// Branch staff record covering both trainers and branch managers.
typedef struct
{
  id_t id;
  char full_name[FULL_NAME_BUFFER_SIZE];
  char email[EMAIL_BUFFER_SIZE];
  char phone_number[PHONE_BUFFER_SIZE];
  char gym_branch[BRANCH_NAME_BUFFER_SIZE];
  char username[USERNAME_BUFFER_SIZE];
  char password_hash[PASSWORD_HASH_BUFFER_SIZE];
  datetime_t joined_at;
  staff_role_t role;
} branch_staff_t;

// Subscription plan with fee amount and payment interval.
typedef struct
{
  unsigned int payable_amount;
  unsigned int interval_days;
} subscription_plan_t;

// Lifecycle status of a gym member's membership.
typedef unsigned char membership_status_t;
#define MEMBERSHIP_ON_HOLD 0
#define MEMBERSHIP_ACTIVE 1
#define MEMBERSHIP_SUSPENDED 2
#define MEMBERSHIP_CANCELLED 3

// Gym member record with plan, dues, and membership status.
typedef struct
{
  id_t id;
  char full_name[FULL_NAME_BUFFER_SIZE];
  char email[EMAIL_BUFFER_SIZE];
  char phone_number[PHONE_BUFFER_SIZE];
  char gym_branch[BRANCH_NAME_BUFFER_SIZE];
  char username[USERNAME_BUFFER_SIZE];
  char password_hash[PASSWORD_HASH_BUFFER_SIZE];
  datetime_t joined_at;
  datetime_t last_payment_date;
  unsigned int due_amount;
  subscription_plan_t plan;
  membership_status_t status;
} gym_member_t;

// Payment method: cash or digital.
typedef unsigned char transaction_t;
#define CASH_TRANSACTION 0
#define DIGITAL_TRANSACTION 1

// Lifecycle status of a payment.
typedef unsigned char payment_status_t;
#define PAYMENT_PENDING 0
#define PAYMENT_COMPLETED 1
#define PAYMENT_FAILED 2
#define PAYMENT_INVALID 3

// Payment made by a gym member; each belongs to exactly one member.
typedef struct
{
  id_t id;
  id_t gym_member_id;
  unsigned int amount;
  datetime_t transaction_time;
  transaction_t transaction_type;
  char transaction_id[TRX_ID_BUFFER_SIZE];
  payment_status_t status;
} payment_t;

// Arguments carried to record a digital payment.
typedef struct
{
  id_t request_id;
  id_t gym_member_id;
  unsigned int amount;
  datetime_t transaction_time;
  char transaction_id[TRX_ID_BUFFER_SIZE];
  payment_status_t status;
} digital_payment_request_t;

// Read-only profile view of a gym member.
typedef struct
{
  id_t id;
  char full_name[FULL_NAME_BUFFER_SIZE];
  char email[EMAIL_BUFFER_SIZE];
  char phone_number[PHONE_BUFFER_SIZE];
  char gym_branch[BRANCH_NAME_BUFFER_SIZE];
  char username[USERNAME_BUFFER_SIZE];
  datetime_t joined_at;
  subscription_plan_t plan;
} gym_member_profile_t;

// Record of a member suspension with reason and dates.
typedef struct
{
  id_t id;
  id_t gym_member_id;
  char reason[REASON_BUFFER_SIZE];
  datetime_t suspension_date;
  datetime_t unsuspension_date;
} suspension_record_t;

// Lost or found item report, resolved by a manager or the system administrator.
typedef struct
{
  id_t id;
  char description[DESCRIPTION_BUFFER_SIZE];
  char reporter_username[USERNAME_BUFFER_SIZE];
  char gym_branch[BRANCH_NAME_BUFFER_SIZE];
  datetime_t reported_at;
  char resolver_username[USERNAME_BUFFER_SIZE];
} lost_and_found_record_t;

// Top-level role of a logged-in user.
typedef unsigned char user_role_t;
#define USER_ROLE_SYSADMIN 0
#define USER_ROLE_BRANCH_MANAGER 1
#define USER_ROLE_TRAINER 2
#define USER_ROLE_MEMBER 3

// Context of the logged-in user; gates access by role and branch.
typedef struct
{
  user_role_t role;
  id_t user_id;
  char username[USERNAME_BUFFER_SIZE];
  char branch_name[BRANCH_NAME_BUFFER_SIZE];
} session_t;

#endif

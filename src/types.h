#ifndef GYMTRAC_TYPES_H
#define GYMTRAC_TYPES_H

#include <time.h>

#include "settings.h"

// Common numeric id type shared by every record.
typedef unsigned long int id_t;

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
  time_t joined_at;
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
  time_t joined_at;
  time_t last_payment_date;
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

// Payment details shared by cash and digital payments.
typedef struct
{
  id_t id;
  unsigned int amount;
  time_t transaction_time;
  transaction_t transaction_type;
  char transaction_id[TRX_ID_BUFFER_SIZE];
  payment_status_t status;
} payment_t;

// Join record linking a gym member to a payment.
typedef struct
{
  id_t gym_member_id;
  id_t payment_id;
} payment_record_t;

// Arguments carried to record a digital payment.
typedef struct
{
  id_t request_id;
  id_t gym_member_id;
  unsigned int amount;
  time_t transaction_time;
  char transaction_id[TRX_ID_BUFFER_SIZE];
  payment_status_t status;
} digital_payment_request_t;

// Lifecycle status of an approval request.
typedef unsigned char request_status_t;
#define REQUEST_REQUESTED 0
#define REQUEST_APPROVED 1
#define REQUEST_REJECTED 2

// Trainer request to change a member's status, resolved by a manager.
typedef struct
{
  id_t request_id;
  id_t gym_member_id;
  id_t requested_by_staff_id;
  id_t resolved_by_staff_id;
  char reason[REASON_BUFFER_SIZE];
  membership_status_t new_membership_status;
  request_status_t status;
  time_t created_at;
} membership_status_change_request_t;

// Member request to change their plan, resolved by branch staff.
typedef struct
{
  id_t request_id;
  id_t gym_member_id;
  subscription_plan_t new_plan;
  request_status_t status;
  time_t created_at;
} subscription_plan_change_request_t;

// Member request to edit profile fields, approved or rejected by staff.
typedef struct
{
  char full_name[FULL_NAME_BUFFER_SIZE];
  char email[EMAIL_BUFFER_SIZE];
  char phone_number[PHONE_BUFFER_SIZE];
  char gym_branch[BRANCH_NAME_BUFFER_SIZE];
  char username[USERNAME_BUFFER_SIZE];
  request_status_t status;
} profile_edit_request_t;

// Read-only profile view of a gym member.
typedef struct
{
  id_t id;
  char full_name[FULL_NAME_BUFFER_SIZE];
  char email[EMAIL_BUFFER_SIZE];
  char phone_number[PHONE_BUFFER_SIZE];
  char gym_branch[BRANCH_NAME_BUFFER_SIZE];
  char username[USERNAME_BUFFER_SIZE];
  time_t joined_at;
  subscription_plan_t plan;
} gym_member_profile_t;

// Record of a member suspension with reason and dates.
typedef struct
{
  id_t id;
  id_t gym_member_id;
  char reason[REASON_BUFFER_SIZE];
  time_t suspension_date;
  time_t unsuspension_date;
} suspension_record_t;

// Lost or found item report, resolved by staff.
typedef struct
{
  id_t id;
  char description[DESCRIPTION_BUFFER_SIZE];
  char reported_by_username[USERNAME_BUFFER_SIZE];
  time_t reported_at;
  id_t resolved_by_staff_id;
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

#ifndef GYMTRAC_TYPES_H
#define GYMTRAC_TYPES_H

#include <time.h>

#include "settings.h"

typedef unsigned long int id_t;

typedef struct {
  id_t id;
  char username[USERNAME_BUFFER_SIZE];
  char password_hash[PASSWORD_HASH_BUFFER_SIZE];
} sysadmin_t;

typedef unsigned char staff_role_t;
#define TRAINER 0
#define BRANCH_MANAGER 1

typedef struct {
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

typedef struct {
  unsigned int payable_amount;
  unsigned int interval_days;
} subscription_plan_t;

typedef unsigned char membership_status_t;
#define ON_HOLD 0
#define ACTIVE 1
#define SUSPENDED 2
#define CANCELLED 3

typedef struct {
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

typedef unsigned char transaction_t;
#define CASH 0
#define DIGITAL 1

typedef unsigned char payment_status_t;
#define PENDING 0
#define COMPLETED 1
#define FAILED 2
#define INVALID 3

typedef struct {
  id_t id;
  unsigned int amount;
  time_t transaction_time;
  transaction_t transaction_type;
  char transaction_id[TRANSACTION_ID_BUFFER_SIZE];
  payment_status_t status;
} payment_t;

typedef struct {
  id_t gym_member_id;
  id_t payment_id;
} payment_record_t;

typedef struct {
  id_t request_id;
  id_t gym_member_id;
  unsigned int amount;
  time_t transaction_time;
  char transaction_id[TRANSACTION_ID_BUFFER_SIZE];
  payment_status_t status;
} digital_payment_request_t;

typedef unsigned char request_status_t;
#define REQUESTED 0
#define APPROVED 1
#define REJECTED 2

typedef struct {
  id_t request_id;
  id_t gym_member_id;
  id_t requested_by_staff_id;
  id_t resolved_by_staff_id;
  char reason[REASON_BUFFER_SIZE];
  membership_status_t new_membership_status;
  request_status_t status;
  time_t created_at;
} membership_status_change_request_t;

typedef struct {
  id_t request_id;
  id_t gym_member_id;
  subscription_plan_t new_plan;
  request_status_t status;
  time_t created_at;
} subscription_plan_change_request_t;

typedef struct {
  char full_name[FULL_NAME_BUFFER_SIZE];
  char email[EMAIL_BUFFER_SIZE];
  char phone_number[PHONE_BUFFER_SIZE];
  char gym_branch[BRANCH_NAME_BUFFER_SIZE];
  char username[USERNAME_BUFFER_SIZE];
  request_status_t status;
} profile_edit_request_t;

typedef struct {
  id_t id;
  char full_name[FULL_NAME_BUFFER_SIZE];
  char email[EMAIL_BUFFER_SIZE];
  char phone_number[PHONE_BUFFER_SIZE];
  char gym_branch[BRANCH_NAME_BUFFER_SIZE];
  char username[USERNAME_BUFFER_SIZE];
  time_t joined_at;
  subscription_plan_t plan;
} gym_member_profile_t;

typedef struct {
  id_t id;
  id_t gym_member_id;
  char reason[REASON_BUFFER_SIZE];
  time_t suspension_date;
  time_t unsuspension_date;
} suspension_record_t;

typedef struct {
  id_t id;
  char description[DESCRIPTION_BUFFER_SIZE];
  char reported_by_username[USERNAME_BUFFER_SIZE];
  time_t reported_at;
  id_t resolved_by_staff_id;
} lost_and_found_record_t;

typedef unsigned char user_role_t;
#define USER_ROLE_SYSADMIN 0
#define USER_ROLE_BRANCH_MANAGER 1
#define USER_ROLE_TRAINER 2
#define USER_ROLE_MEMBER 3

typedef struct {
  user_role_t role;
  id_t user_id;
  char username[USERNAME_BUFFER_SIZE];
  char branch_name[BRANCH_NAME_BUFFER_SIZE];
} session_t;

#endif

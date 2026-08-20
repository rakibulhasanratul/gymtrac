#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "../settings.h"
#include "../types.h"
#include "../utils/file_util.h"
#include "../utils/string_util.h"
#include "user.h"

static sysadmin_t sysadmins[MAX_SYSTEM_ADMINS];
static int sysadmin_count;
static id_t next_sysadmin_id;

static branch_staff_t branch_staff_list[MAX_BRANCH_MANAGERS + MAX_TRAINERS];
static int branch_staff_count;
static id_t next_branch_staff_id;

static gym_member_t gym_members[MAX_GYM_MEMBERS];
static int gym_member_count;
static id_t next_gym_member_id;

/**
 * Appends a sysadmin record as a pipe-delimited line to the data file.
 */
static bool persist_sysadmin(const sysadmin_t *record_payload)
{
  char path[PATH_BUFFER_SIZE];
  build_file_path(SYSDADMINS_FILENAME, path, PATH_BUFFER_SIZE);

  FILE *file = fopen(path, "a");
  if (file == NULL)
    return false;

  char line[LINE_BUFFER_SIZE];
  snprintf(line, LINE_BUFFER_SIZE, "%lu|%s|%s", (unsigned long)record_payload->id, record_payload->username,
           record_payload->password_hash);

  bool success = write_line_to_file(file, line);
  fclose(file);
  return success;
}

/**
 * Appends a branch staff record as a pipe-delimited line to the data file.
 */
static bool persist_branch_staff(const branch_staff_t *record_payload)
{
  char path[PATH_BUFFER_SIZE];
  build_file_path(BRANCH_STAFF_FILENAME, path, PATH_BUFFER_SIZE);

  FILE *file = fopen(path, "a");
  if (file == NULL)
    return false;

  char line[LINE_BUFFER_SIZE];
  snprintf(line, LINE_BUFFER_SIZE, "%lu|%s|%s|%s|%s|%s|%s|%ld|%d", (unsigned long)record_payload->id,
           record_payload->full_name, record_payload->email, record_payload->phone_number, record_payload->gym_branch,
           record_payload->username, record_payload->password_hash, (long)record_payload->joined_at,
           (int)record_payload->role);

  bool success = write_line_to_file(file, line);
  fclose(file);
  return success;
}

/**
 * Appends a gym member record as a pipe-delimited line to the data file.
 */
static bool persist_gym_member(const gym_member_t *record_payload)
{
  char path[PATH_BUFFER_SIZE];
  build_file_path(GYM_MEMBERS_FILENAME, path, PATH_BUFFER_SIZE);

  FILE *file = fopen(path, "a");
  if (file == NULL)
    return false;

  char line[LINE_BUFFER_SIZE];
  snprintf(line, LINE_BUFFER_SIZE, "%lu|%s|%s|%s|%s|%s|%s|%ld|%ld|%u|%u|%u|%d", (unsigned long)record_payload->id,
           record_payload->full_name, record_payload->email, record_payload->phone_number, record_payload->gym_branch,
           record_payload->username, record_payload->password_hash, (long)record_payload->joined_at,
           (long)record_payload->last_payment_date, record_payload->due_amount, record_payload->plan.payable_amount,
           record_payload->plan.interval_days, (int)record_payload->status);

  bool success = write_line_to_file(file, line);
  fclose(file);
  return success;
}

/**
 * Splits a pipe-delimited line into field buffers and returns the field count.
 */
static int split_record_line(const char line[], char parts_destination[][FIELD_BUFFER_SIZE])
{
  // this is just to prevent seg faults
  char *parts[MAX_RECORD_FIELDS];
  for (int i = 0; i < MAX_RECORD_FIELDS; i++)
    parts[i] = parts_destination[i];

  return split(line, FIELD_DELIMITER, parts, MAX_RECORD_FIELDS, FIELD_BUFFER_SIZE);
}

/**
 * Parses a pipe-delimited line into a sysadmin record.
 * Returns true on success, false if the field count is wrong.
 */
static bool parse_sysadmin_line(const char line[], sysadmin_t *destination)
{
  char parts[MAX_RECORD_FIELDS][FIELD_BUFFER_SIZE];
  int count = split_record_line(line, parts);
  if (count != 3)
    return false;

  destination->id = string_to_unsigned_long_int(parts[0]);
  strcpy(destination->username, parts[1]);
  strcpy(destination->password_hash, parts[2]);
  return true;
}

/**
 * Parses a pipe-delimited line into a branch staff record.
 * Returns true on success, false if the field count is wrong.
 */
static bool parse_branch_staff_line(const char line[], branch_staff_t *destination)
{
  char parts[MAX_RECORD_FIELDS][FIELD_BUFFER_SIZE];
  int count = split_record_line(line, parts);
  if (count != 9)
    return false;

  destination->id = string_to_unsigned_long_int(parts[0]);
  strcpy(destination->full_name, parts[1]);
  strcpy(destination->email, parts[2]);
  strcpy(destination->phone_number, parts[3]);
  strcpy(destination->gym_branch, parts[4]);
  strcpy(destination->username, parts[5]);
  strcpy(destination->password_hash, parts[6]);
  destination->joined_at = (time_t)string_to_unsigned_long_int(parts[7]);
  destination->role = (staff_role_t)string_to_unsigned_int(parts[8]);
  return true;
}

/**
 * Parses a pipe-delimited line into a gym member record.
 * Returns true on success, false if the field count is wrong.
 */
static bool parse_gym_member_line(const char line[], gym_member_t *destination)
{
  char parts[MAX_RECORD_FIELDS][FIELD_BUFFER_SIZE];
  int count = split_record_line(line, parts);
  if (count != 13)
    return false;

  destination->id = string_to_unsigned_long_int(parts[0]);
  strcpy(destination->full_name, parts[1]);
  strcpy(destination->email, parts[2]);
  strcpy(destination->phone_number, parts[3]);
  strcpy(destination->gym_branch, parts[4]);
  strcpy(destination->username, parts[5]);
  strcpy(destination->password_hash, parts[6]);
  destination->joined_at = (time_t)string_to_unsigned_long_int(parts[7]);
  destination->last_payment_date = (time_t)string_to_unsigned_long_int(parts[8]);
  destination->due_amount = string_to_unsigned_int(parts[9]);
  destination->plan.payable_amount = string_to_unsigned_int(parts[10]);
  destination->plan.interval_days = string_to_unsigned_int(parts[11]);
  destination->status = (membership_status_t)string_to_unsigned_int(parts[12]);
  return true;
}

int load_sysadmins()
{
  sysadmin_count = 0;

  char path[PATH_BUFFER_SIZE];
  build_file_path(SYSDADMINS_FILENAME, path, PATH_BUFFER_SIZE);

  FILE *file = fopen(path, "r");
  if (file == NULL)
  {
    next_sysadmin_id = 1;
    return 0;
  }

  char line[LINE_BUFFER_SIZE];
  while (sysadmin_count < MAX_SYSTEM_ADMINS && read_line_from_file(file, line, LINE_BUFFER_SIZE))
  {
    if (strlen(line) > 0 && parse_sysadmin_line(line, &sysadmins[sysadmin_count]))
      sysadmin_count++;
  }

  fclose(file);
  next_sysadmin_id = sysadmin_count > 0 ? sysadmins[sysadmin_count - 1].id + 1 : 1;
  return sysadmin_count;
}

int load_branch_staff()
{
  branch_staff_count = 0;

  char path[PATH_BUFFER_SIZE];
  build_file_path(BRANCH_STAFF_FILENAME, path, PATH_BUFFER_SIZE);

  FILE *file = fopen(path, "r");
  if (file == NULL)
  {
    next_branch_staff_id = 1;
    return 0;
  }

  char line[LINE_BUFFER_SIZE];
  int capacity = MAX_BRANCH_MANAGERS + MAX_TRAINERS;
  while (branch_staff_count < capacity && read_line_from_file(file, line, LINE_BUFFER_SIZE))
  {
    if (strlen(line) > 0 && parse_branch_staff_line(line, &branch_staff_list[branch_staff_count]))
      branch_staff_count++;
  }

  fclose(file);
  next_branch_staff_id = branch_staff_count > 0 ? branch_staff_list[branch_staff_count - 1].id + 1 : 1;
  return branch_staff_count;
}

int load_gym_members()
{
  gym_member_count = 0;

  char path[PATH_BUFFER_SIZE];
  build_file_path(GYM_MEMBERS_FILENAME, path, PATH_BUFFER_SIZE);

  FILE *file = fopen(path, "r");
  if (file == NULL)
  {
    next_gym_member_id = 1;
    return 0;
  }

  char line[LINE_BUFFER_SIZE];
  while (gym_member_count < MAX_GYM_MEMBERS && read_line_from_file(file, line, LINE_BUFFER_SIZE))
  {
    if (strlen(line) > 0 && parse_gym_member_line(line, &gym_members[gym_member_count]))
      gym_member_count++;
  }

  fclose(file);
  next_gym_member_id = gym_member_count > 0 ? gym_members[gym_member_count - 1].id + 1 : 1;
  return gym_member_count;
}

id_t create_sysadmin(const char username[], const char password_hash[])
{
  if (username == NULL || strlen(username) == 0)
    return 0;

  if (password_hash == NULL || strlen(password_hash) == 0)
    return 0;

  if (sysadmin_count >= MAX_SYSTEM_ADMINS)
    return 0;

  if (username_exists(username))
    return 0;

  sysadmin_t record;
  record.id = next_sysadmin_id++;
  strcpy(record.username, username);
  strcpy(record.password_hash, password_hash);

  if (!persist_sysadmin(&record))
    return 0;

  sysadmins[sysadmin_count] = record;
  sysadmin_count++;
  return record.id;
}

id_t create_branch_staff(const char full_name[], const char email[], const char phone_number[], const char gym_branch[],
                         const char username[], const char password_hash[], staff_role_t role)
{
  if (full_name == NULL || strlen(full_name) == 0)
    return 0;

  if (email == NULL || strlen(email) == 0)
    return 0;

  if (phone_number == NULL || strlen(phone_number) == 0)
    return 0;

  if (gym_branch == NULL || strlen(gym_branch) == 0)
    return 0;

  if (username == NULL || strlen(username) == 0)
    return 0;

  if (password_hash == NULL || strlen(password_hash) == 0)
    return 0;

  if (username_exists(username))
    return 0;

  int capacity = MAX_BRANCH_MANAGERS + MAX_TRAINERS;
  if (branch_staff_count >= capacity)
    return 0;

  branch_staff_t record;
  record.id = next_branch_staff_id++;

  strcpy(record.full_name, full_name);
  strcpy(record.email, email);
  strcpy(record.phone_number, phone_number);
  strcpy(record.gym_branch, gym_branch);
  strcpy(record.username, username);
  strcpy(record.password_hash, password_hash);
  record.joined_at = time(NULL);
  record.role = role;

  if (!persist_branch_staff(&record))
    return 0;

  branch_staff_list[branch_staff_count] = record;
  branch_staff_count++;
  return record.id;
}

id_t create_gym_member(const char full_name[], const char email[], const char phone_number[], const char gym_branch[],
                       const char username[], const char password_hash[], subscription_plan_t plan_payload,
                       membership_status_t status)
{
  if (full_name == NULL || strlen(full_name) == 0)
    return 0;

  if (email == NULL || strlen(email) == 0)
    return 0;

  if (phone_number == NULL || strlen(phone_number) == 0)
    return 0;

  if (gym_branch == NULL || strlen(gym_branch) == 0)
    return 0;

  if (username == NULL || strlen(username) == 0)
    return 0;

  if (password_hash == NULL || strlen(password_hash) == 0)
    return 0;

  if (username_exists(username))
    return 0;

  if (gym_member_count >= MAX_GYM_MEMBERS)
    return 0;

  gym_member_t record;
  record.id = next_gym_member_id++;

  strcpy(record.full_name, full_name);
  strcpy(record.email, email);
  strcpy(record.phone_number, phone_number);
  strcpy(record.gym_branch, gym_branch);
  strcpy(record.username, username);
  strcpy(record.password_hash, password_hash);
  record.joined_at = time(NULL);
  record.last_payment_date = 0;
  record.due_amount = 0;
  record.plan = plan_payload;
  record.status = status;

  if (!persist_gym_member(&record))
    return 0;

  gym_members[gym_member_count] = record;
  gym_member_count++;
  return record.id;
}

bool username_exists(const char username[])
{
  if (username == NULL || strlen(username) == 0)
    return false;

  for (int i = 0; i < sysadmin_count; i++)
  {
    if (strcmp(sysadmins[i].username, username) == 0)
      return true;
  }

  for (int i = 0; i < branch_staff_count; i++)
  {
    if (strcmp(branch_staff_list[i].username, username) == 0)
      return true;
  }

  for (int i = 0; i < gym_member_count; i++)
  {
    if (strcmp(gym_members[i].username, username) == 0)
      return true;
  }

  return false;
}

int branch_manager_count(const char branch_name[])
{
  if (branch_name == NULL || strlen(branch_name) == 0)
    return 0;

  int count = 0;
  for (int i = 0; i < branch_staff_count; i++)
  {
    if (branch_staff_list[i].role == BRANCH_MANAGER && strcmp(branch_staff_list[i].gym_branch, branch_name) == 0)
      count++;
  }
  return count;
}

int branch_trainer_count(const char branch_name[])
{
  if (branch_name == NULL || strlen(branch_name) == 0)
    return 0;

  int count = 0;
  for (int i = 0; i < branch_staff_count; i++)
  {
    if (branch_staff_list[i].role == TRAINER && strcmp(branch_staff_list[i].gym_branch, branch_name) == 0)
      count++;
  }
  return count;
}

int branch_member_count(const char branch_name[])
{
  if (branch_name == NULL || strlen(branch_name) == 0)
    return 0;

  int count = 0;
  for (int i = 0; i < gym_member_count; i++)
  {
    if (strcmp(gym_members[i].gym_branch, branch_name) == 0)
      count++;
  }
  return count;
}

sysadmin_t *get_sysadmin_by_id(id_t id)
{
  for (int i = 0; i < sysadmin_count; i++)
  {
    if (sysadmins[i].id == id)
      return &sysadmins[i];
  }
  return NULL;
}

sysadmin_t *get_sysadmin_by_username(const char username[])
{
  if (username == NULL || strlen(username) == 0)
    return NULL;

  for (int i = 0; i < sysadmin_count; i++)
  {
    if (strcmp(sysadmins[i].username, username) == 0)
      return &sysadmins[i];
  }
  return NULL;
}

branch_staff_t *get_branch_staff_by_id(id_t id)
{
  for (int i = 0; i < branch_staff_count; i++)
  {
    if (branch_staff_list[i].id == id)
      return &branch_staff_list[i];
  }
  return NULL;
}

branch_staff_t *get_branch_staff_by_username(const char username[])
{
  if (username == NULL || strlen(username) == 0)
    return NULL;

  for (int i = 0; i < branch_staff_count; i++)
  {
    if (strcmp(branch_staff_list[i].username, username) == 0)
      return &branch_staff_list[i];
  }
  return NULL;
}

gym_member_t *get_gym_member_by_id(id_t id)
{
  for (int i = 0; i < gym_member_count; i++)
  {
    if (gym_members[i].id == id)
      return &gym_members[i];
  }
  return NULL;
}

gym_member_t *get_gym_member_by_username(const char username[])
{
  if (username == NULL || strlen(username) == 0)
    return NULL;

  for (int i = 0; i < gym_member_count; i++)
  {
    if (strcmp(gym_members[i].username, username) == 0)
      return &gym_members[i];
  }
  return NULL;
}

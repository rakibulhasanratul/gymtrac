#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "../settings.h"
#include "../types.h"
#include "../utils/datetime_utils.h"
#include "../utils/file_util.h"
#include "../utils/string_util.h"
#include "user.h"

// Alias so format strings read "%lu" SEP "%s" instead of repeating FIELD_DELIMITER_STRING everywhere.
#define SEP FIELD_DELIMITER_STRING

static sysadmin_t sysadmins[MAX_SYSTEM_ADMINS];
static int sysadmin_count;
static id_t next_sysadmin_id;

static branch_staff_t branch_staffs[MAX_BRANCH_MANAGERS + MAX_TRAINERS];
static int branch_staff_count;
static id_t next_branch_staff_id;

static gym_member_t gym_members[MAX_GYM_MEMBERS];
static int gym_member_count;
static id_t next_gym_member_id;

// Formats a sysadmin record as a delimiter-separated line.
static inline void format_sysadmin_line(const sysadmin_t record_payload, char *line_destination)
{
  snprintf(
    line_destination, LINE_BUFFER_SIZE, "%lu" SEP "%s" SEP "%s", (unsigned long)record_payload.id,
    record_payload.username, record_payload.password_hash
  );
}

// Appends a sysadmin record as a delimiter-separated line to the data file.
static bool persist_sysadmin(const sysadmin_t record_payload)
{
  FILE *file = fopen(SYSADMINS_FILE_PATH, "a");
  if (file == NULL)
  {
    LOG_ERROR("Error: Failed to open sysadmin data file.");
    return false;
  }

  char line[LINE_BUFFER_SIZE];
  format_sysadmin_line(record_payload, line);

  bool success = write_line_to_file(file, line);
  fclose(file);
  return success;
}

// Formats a branch staff record as a delimiter-separated line.
static inline void format_branch_staff_line(const branch_staff_t record_payload, char *line_destination)
{
  snprintf(
    line_destination, LINE_BUFFER_SIZE, "%lu" SEP "%s" SEP "%s" SEP "%s" SEP "%s" SEP "%s" SEP "%s" SEP "%lld" SEP "%d",
    (unsigned long)record_payload.id, record_payload.full_name, record_payload.email, record_payload.phone_number,
    record_payload.gym_branch, record_payload.username, record_payload.password_hash,
    datetime_to_seconds(record_payload.joined_at), (int)record_payload.role
  );
}

// Formats a gym member record as a delimiter-separated line.
static inline void format_gym_member_line(const gym_member_t record_payload, char *line_destination)
{
  snprintf(
    line_destination, LINE_BUFFER_SIZE,
    "%lu" SEP "%s" SEP "%s" SEP "%s" SEP "%s" SEP "%s" SEP "%s" SEP "%lld" SEP "%lld" SEP "%u" SEP "%u" SEP "%u" SEP
    "%d",
    (unsigned long)record_payload.id, record_payload.full_name, record_payload.email, record_payload.phone_number,
    record_payload.gym_branch, record_payload.username, record_payload.password_hash,
    datetime_to_seconds(record_payload.joined_at), datetime_to_seconds(record_payload.last_payment_date),
    record_payload.due_amount, record_payload.plan.payable_amount, record_payload.plan.interval_days,
    (int)record_payload.status
  );
}

// Appends a branch staff record as a delimiter-separated line to the data file.
static bool persist_branch_staff(const branch_staff_t record_payload)
{
  FILE *file = fopen(BRANCH_STAFF_FILE_PATH, "a");
  if (file == NULL)
  {
    LOG_ERROR("Error: Failed to open branch staff data file.");
    return false;
  }

  char line[LINE_BUFFER_SIZE];
  format_branch_staff_line(record_payload, line);

  bool success = write_line_to_file(file, line);
  fclose(file);
  return success;
}

// Appends a gym member record as a delimiter-separated line to the data file.
static bool persist_gym_member(const gym_member_t record_payload)
{
  FILE *file = fopen(GYM_MEMBERS_FILE_PATH, "a");
  if (file == NULL)
  {
    LOG_ERROR("Error: Failed to open gym member data file.");
    return false;
  }

  char line[LINE_BUFFER_SIZE];
  format_gym_member_line(record_payload, line);

  bool success = write_line_to_file(file, line);
  fclose(file);
  return success;
}

// Removes the branch staff record at index from memory by shifting later records left.
static inline void remove_branch_staff_at(int index)
{
  for (int i = index; i < branch_staff_count - 1; i++) branch_staffs[i] = branch_staffs[i + 1];

  branch_staff_count--;
}

// Removes the gym member record at index from memory by shifting later records left.
static inline void remove_gym_member_at(int index)
{
  for (int i = index; i < gym_member_count - 1; i++) gym_members[i] = gym_members[i + 1];

  gym_member_count--;
}

// Rewrites the branch staff file from memory, skipping the record at index.
//
// Used by delete_branch_staff; the removed record never persists, matching
// creation's file-first ordering.
static bool rewrite_all_branch_staff_to_file_without_index(int index)
{
  FILE *file = fopen(BRANCH_STAFF_FILE_PATH, "w");
  if (file == NULL)
  {
    LOG_ERROR("Error: Failed to open branch staff file for writing.");
    return false;
  }

  char line[LINE_BUFFER_SIZE];
  for (int i = 0; i < branch_staff_count; i++)
  {
    if (i == index) continue;

    format_branch_staff_line(branch_staffs[i], line);
    if (!write_line_to_file(file, line))
    {
      fclose(file);
      LOG_ERROR("Error: Failed to write branch staff record.");
      return false;
    }
  }

  fclose(file);
  return true;
}

// Rewrites the gym members file from memory, skipping the record at index.
//
// Used by delete_gym_member; the removed record never persists, matching
// creation's file-first ordering.
static bool rewrite_all_gym_members_to_file_without_index(int index)
{
  FILE *file = fopen(GYM_MEMBERS_FILE_PATH, "w");
  if (file == NULL)
  {
    LOG_ERROR("Error: Failed to open gym members file for writing.");
    return false;
  }

  char line[LINE_BUFFER_SIZE];
  for (int i = 0; i < gym_member_count; i++)
  {
    if (i == index) continue;

    format_gym_member_line(gym_members[i], line);
    if (!write_line_to_file(file, line))
    {
      fclose(file);
      LOG_ERROR("Error: Failed to write gym member record.");
      return false;
    }
  }

  fclose(file);
  return true;
}

// Rewrites the entire branch staff file from memory.
//
// Used by update_branch_staff so the persisted file reflects the
// modified record.
static bool rewrite_all_branch_staffs_to_file()
{
  FILE *file = fopen(BRANCH_STAFF_FILE_PATH, "w");
  if (file == NULL)
  {
    LOG_ERROR("Error: Failed to open branch staff file for writing.");
    return false;
  }

  char line[LINE_BUFFER_SIZE];
  for (int i = 0; i < branch_staff_count; i++)
  {
    format_branch_staff_line(branch_staffs[i], line);
    if (!write_line_to_file(file, line))
    {
      fclose(file);
      LOG_ERROR("Error: Failed to write branch staff record.");
      return false;
    }
  }

  fclose(file);
  return true;
}

// Rewrites the entire gym members file from memory.
//
// Used by update_gym_member so the persisted file reflects the
// modified record.
static bool rewrite_all_gym_members_to_file()
{
  FILE *file = fopen(GYM_MEMBERS_FILE_PATH, "w");
  if (file == NULL)
  {
    LOG_ERROR("Error: Failed to open gym members file for writing.");
    return false;
  }

  char line[LINE_BUFFER_SIZE];
  for (int i = 0; i < gym_member_count; i++)
  {
    format_gym_member_line(gym_members[i], line);
    if (!write_line_to_file(file, line))
    {
      fclose(file);
      LOG_ERROR("Error: Failed to write gym member record.");
      return false;
    }
  }

  fclose(file);
  return true;
}

// Splits a pipe-delimited record line into field buffers and returns the
// field count.
static inline int split_record_line(const char line[], char parts_destination[][FIELD_BUFFER_SIZE])
{
  return split(line, FIELD_DELIMITER, parts_destination, MAX_RECORD_FIELDS);
}

// Parses a pipe-delimited line into a sysadmin record.
// Returns true on success, false if the field count is wrong.
static inline bool parse_sysadmin_line(const char line[], sysadmin_t *destination)
{
  char parts[MAX_RECORD_FIELDS][FIELD_BUFFER_SIZE];
  int count = split_record_line(line, parts);
  if (count != 3) return false;

  destination->id = string_to_unsigned_long_int(parts[0]);
  strcpy(destination->username, parts[1]);
  strcpy(destination->password_hash, parts[2]);
  return true;
}

// Parses a pipe-delimited line into a branch staff record.
// Returns true on success, false if the field count is wrong.
static inline bool parse_branch_staff_line(const char line[], branch_staff_t *destination)
{
  char parts[MAX_RECORD_FIELDS][FIELD_BUFFER_SIZE];
  int count = split_record_line(line, parts);
  if (count != 9) return false;

  destination->id = string_to_unsigned_long_int(parts[0]);
  strcpy(destination->full_name, parts[1]);
  strcpy(destination->email, parts[2]);
  strcpy(destination->phone_number, parts[3]);
  strcpy(destination->gym_branch, parts[4]);
  strcpy(destination->username, parts[5]);
  strcpy(destination->password_hash, parts[6]);
  destination->joined_at = datetime_from_seconds((long long)string_to_unsigned_long_int(parts[7]));
  destination->role = (staff_role_t)string_to_unsigned_int(parts[8]);
  return true;
}

// Parses a pipe-delimited line into a gym member record.
// Returns true on success, false if the field count is wrong.
static inline bool parse_gym_member_line(const char line[], gym_member_t *destination)
{
  char parts[MAX_RECORD_FIELDS][FIELD_BUFFER_SIZE];
  int count = split_record_line(line, parts);
  if (count != 13) return false;

  destination->id = string_to_unsigned_long_int(parts[0]);
  strcpy(destination->full_name, parts[1]);
  strcpy(destination->email, parts[2]);
  strcpy(destination->phone_number, parts[3]);
  strcpy(destination->gym_branch, parts[4]);
  strcpy(destination->username, parts[5]);
  strcpy(destination->password_hash, parts[6]);
  destination->joined_at = datetime_from_seconds((long long)string_to_unsigned_long_int(parts[7]));
  destination->last_payment_date = datetime_from_seconds((long long)string_to_unsigned_long_int(parts[8]));
  destination->due_amount = string_to_unsigned_int(parts[9]);
  destination->plan.payable_amount = string_to_unsigned_int(parts[10]);
  destination->plan.interval_days = string_to_unsigned_int(parts[11]);
  destination->status = (membership_status_t)string_to_unsigned_int(parts[12]);
  return true;
}

int load_sysadmins()
{
  sysadmin_count = 0;

  FILE *file = fopen(SYSADMINS_FILE_PATH, "r");
  if (file == NULL)
  {
    next_sysadmin_id = 1;
    return 0;
  }

  char line[LINE_BUFFER_SIZE];
  while (sysadmin_count < MAX_SYSTEM_ADMINS && read_line_from_file(file, line, LINE_BUFFER_SIZE))
  {
    if (!is_blank_string(line) && parse_sysadmin_line(line, &sysadmins[sysadmin_count])) sysadmin_count++;
  }

  fclose(file);
  next_sysadmin_id = sysadmin_count > 0 ? sysadmins[sysadmin_count - 1].id + 1 : 1;
  return sysadmin_count;
}

int load_branch_staff()
{
  branch_staff_count = 0;

  FILE *file = fopen(BRANCH_STAFF_FILE_PATH, "r");
  if (file == NULL)
  {
    next_branch_staff_id = 1;
    return 0;
  }

  char line[LINE_BUFFER_SIZE];
  int capacity = MAX_BRANCH_MANAGERS + MAX_TRAINERS;
  while (branch_staff_count < capacity && read_line_from_file(file, line, LINE_BUFFER_SIZE))
  {
    if (!is_blank_string(line) && parse_branch_staff_line(line, &branch_staffs[branch_staff_count]))
      branch_staff_count++;
  }

  fclose(file);
  next_branch_staff_id = branch_staff_count > 0 ? branch_staffs[branch_staff_count - 1].id + 1 : 1;
  return branch_staff_count;
}

int load_gym_members()
{
  gym_member_count = 0;

  FILE *file = fopen(GYM_MEMBERS_FILE_PATH, "r");
  if (file == NULL)
  {
    next_gym_member_id = 1;
    return 0;
  }

  char line[LINE_BUFFER_SIZE];
  while (gym_member_count < MAX_GYM_MEMBERS && read_line_from_file(file, line, LINE_BUFFER_SIZE))
  {
    if (!is_blank_string(line) && parse_gym_member_line(line, &gym_members[gym_member_count])) gym_member_count++;
  }

  fclose(file);
  next_gym_member_id = gym_member_count > 0 ? gym_members[gym_member_count - 1].id + 1 : 1;
  return gym_member_count;
}

id_t create_sysadmin(const char username[], const char password_hash[])
{
  if (is_blank_string(username))
  {
    LOG_ERROR("Error: Username cannot be empty.");
    return 0;
  }

  if (is_blank_string(password_hash))
  {
    LOG_ERROR("Error: Password hash cannot be empty.");
    return 0;
  }

  if (sysadmin_count >= MAX_SYSTEM_ADMINS)
  {
    LOG_ERROR("Error: Maximum sysadmin count reached.");
    return 0;
  }

  if (username_exists(username))
  {
    LOG_ERROR("Error: Username '%s' already exists.", username);
    return 0;
  }

  sysadmin_t record;
  record.id = next_sysadmin_id++;
  strcpy(record.username, username);
  strcpy(record.password_hash, password_hash);

  if (!persist_sysadmin(record))
  {
    LOG_ERROR("Error: Failed to persist sysadmin record.");
    return 0;
  }

  sysadmins[sysadmin_count] = record;
  sysadmin_count++;
  return record.id;
}

id_t create_branch_staff(
  const char full_name[],
  const char email[],
  const char phone_number[],
  const char gym_branch[],
  const char username[],
  const char password_hash[],
  staff_role_t role
)
{
  if (is_blank_string(full_name))
  {
    LOG_ERROR("Error: Full name cannot be empty.");
    return 0;
  }

  if (is_blank_string(email))
  {
    LOG_ERROR("Error: Email cannot be empty.");
    return 0;
  }

  if (is_blank_string(phone_number))
  {
    LOG_ERROR("Error: Phone number cannot be empty.");
    return 0;
  }

  if (is_blank_string(gym_branch))
  {
    LOG_ERROR("Error: Branch name cannot be empty.");
    return 0;
  }

  if (is_blank_string(username))
  {
    LOG_ERROR("Error: Username cannot be empty.");
    return 0;
  }

  if (is_blank_string(password_hash))
  {
    LOG_ERROR("Error: Password hash cannot be empty.");
    return 0;
  }

  if (username_exists(username))
  {
    LOG_ERROR("Error: Username '%s' already exists.", username);
    return 0;
  }

  int capacity = MAX_BRANCH_MANAGERS + MAX_TRAINERS;
  if (branch_staff_count >= capacity)
  {
    LOG_ERROR("Error: Maximum branch staff count reached.");
    return 0;
  }

  branch_staff_t record;
  record.id = next_branch_staff_id++;

  strcpy(record.full_name, full_name);
  strcpy(record.email, email);
  strcpy(record.phone_number, phone_number);
  strcpy(record.gym_branch, gym_branch);
  strcpy(record.username, username);
  strcpy(record.password_hash, password_hash);
  record.joined_at = now_datetime();
  record.role = role;

  if (!persist_branch_staff(record))
  {
    LOG_ERROR("Error: Failed to persist branch staff record.");
    return 0;
  }

  branch_staffs[branch_staff_count] = record;
  branch_staff_count++;
  return record.id;
}

id_t create_gym_member(
  const char full_name[],
  const char email[],
  const char phone_number[],
  const char gym_branch[],
  const char username[],
  const char password_hash[],
  subscription_plan_t plan_payload,
  membership_status_t status
)
{
  if (is_blank_string(full_name))
  {
    LOG_ERROR("Error: Full name cannot be empty.");
    return 0;
  }

  if (is_blank_string(email))
  {
    LOG_ERROR("Error: Email cannot be empty.");
    return 0;
  }

  if (is_blank_string(phone_number))
  {
    LOG_ERROR("Error: Phone number cannot be empty.");
    return 0;
  }

  if (is_blank_string(gym_branch))
  {
    LOG_ERROR("Error: Branch name cannot be empty.");
    return 0;
  }

  if (is_blank_string(username))
  {
    LOG_ERROR("Error: Username cannot be empty.");
    return 0;
  }

  if (is_blank_string(password_hash))
  {
    LOG_ERROR("Error: Password hash cannot be empty.");
    return 0;
  }

  if (username_exists(username))
  {
    LOG_ERROR("Error: Username '%s' already exists.", username);
    return 0;
  }

  if (gym_member_count >= MAX_GYM_MEMBERS)
  {
    LOG_ERROR("Error: Maximum gym member count reached.");
    return 0;
  }

  gym_member_t record;
  record.id = next_gym_member_id++;

  strcpy(record.full_name, full_name);
  strcpy(record.email, email);
  strcpy(record.phone_number, phone_number);
  strcpy(record.gym_branch, gym_branch);
  strcpy(record.username, username);
  strcpy(record.password_hash, password_hash);
  record.joined_at = now_datetime();
  record.last_payment_date = EMPTY_DATETIME;
  record.due_amount = 0;
  record.plan = plan_payload;
  record.status = status;

  if (!persist_gym_member(record))
  {
    LOG_ERROR("Error: Failed to persist gym member record.");
    return 0;
  }

  gym_members[gym_member_count] = record;
  gym_member_count++;
  return record.id;
}

bool delete_branch_staff(id_t id)
{
  int index = -1;
  for (int i = 0; i < branch_staff_count; i++)
  {
    if (branch_staffs[i].id == id)
    {
      index = i;
      break;
    }
  }

  if (index < 0)
  {
    LOG_ERROR("Error: No staff member found with id %lu.", (unsigned long)id);
    return false;
  }

  if (!rewrite_all_branch_staff_to_file_without_index(index))
  {
    LOG_ERROR("Error: Failed to rewrite branch staff file.");
    return false;
  }

  remove_branch_staff_at(index);
  return true;
}

bool delete_gym_member(id_t id)
{
  int index = -1;
  for (int i = 0; i < gym_member_count; i++)
  {
    if (gym_members[i].id == id)
    {
      index = i;
      break;
    }
  }

  if (index < 0)
  {
    LOG_ERROR("Error: No gym member found with id %lu.", (unsigned long)id);
    return false;
  }

  if (gym_members[index].due_amount != 0)
  {
    LOG_ERROR("Error: Member with id %lu has outstanding dues.", (unsigned long)id);
    return false;
  }

  if (!rewrite_all_gym_members_to_file_without_index(index))
  {
    LOG_ERROR("Error: Failed to rewrite gym members file.");
    return false;
  }

  remove_gym_member_at(index);
  return true;
}

bool username_exists(const char username[])
{
  if (is_blank_string(username)) return false;

  for (int i = 0; i < sysadmin_count; i++)
  {
    if (strcmp(sysadmins[i].username, username) == 0) return true;
  }

  for (int i = 0; i < branch_staff_count; i++)
  {
    if (strcmp(branch_staffs[i].username, username) == 0) return true;
  }

  for (int i = 0; i < gym_member_count; i++)
  {
    if (strcmp(gym_members[i].username, username) == 0) return true;
  }

  return false;
}

int branch_manager_count(const char branch_name[])
{
  if (is_blank_string(branch_name)) return 0;

  int count = 0;
  for (int i = 0; i < branch_staff_count; i++)
  {
    if (branch_staffs[i].role == BRANCH_MANAGER && strcmp(branch_staffs[i].gym_branch, branch_name) == 0) count++;
  }
  return count;
}

int branch_trainer_count(const char branch_name[])
{
  if (is_blank_string(branch_name)) return 0;

  int count = 0;
  for (int i = 0; i < branch_staff_count; i++)
  {
    if (branch_staffs[i].role == TRAINER && strcmp(branch_staffs[i].gym_branch, branch_name) == 0) count++;
  }
  return count;
}

int branch_member_count(const char branch_name[])
{
  if (is_blank_string(branch_name)) return 0;

  int count = 0;
  for (int i = 0; i < gym_member_count; i++)
  {
    if (strcmp(gym_members[i].gym_branch, branch_name) == 0) count++;
  }
  return count;
}

bool get_sysadmin_by_id(id_t id, sysadmin_t *destination)
{
  if (destination == NULL) return false;

  for (int i = 0; i < sysadmin_count; i++)
  {
    if (sysadmins[i].id == id)
    {
      *destination = sysadmins[i];
      return true;
    }
  }
  return false;
}

bool get_sysadmin_by_username(const char username[], sysadmin_t *destination)
{
  if (is_blank_string(username) || destination == NULL) return false;

  for (int i = 0; i < sysadmin_count; i++)
  {
    if (strcmp(sysadmins[i].username, username) == 0)
    {
      *destination = sysadmins[i];
      return true;
    }
  }
  return false;
}

bool get_branch_staff_by_id(id_t id, branch_staff_t *destination)
{
  if (destination == NULL) return false;

  for (int i = 0; i < branch_staff_count; i++)
  {
    if (branch_staffs[i].id == id)
    {
      *destination = branch_staffs[i];
      return true;
    }
  }
  return false;
}

bool get_branch_staff_by_username(const char username[], branch_staff_t *destination)
{
  if (is_blank_string(username) || destination == NULL) return false;

  for (int i = 0; i < branch_staff_count; i++)
  {
    if (strcmp(branch_staffs[i].username, username) == 0)
    {
      *destination = branch_staffs[i];
      return true;
    }
  }
  return false;
}

bool get_gym_member_by_id(id_t id, gym_member_t *destination)
{
  if (destination == NULL) return false;

  for (int i = 0; i < gym_member_count; i++)
  {
    if (gym_members[i].id == id)
    {
      *destination = gym_members[i];
      return true;
    }
  }
  return false;
}

bool get_gym_member_by_username(const char username[], gym_member_t *destination)
{
  if (is_blank_string(username) || destination == NULL) return false;

  for (int i = 0; i < gym_member_count; i++)
  {
    if (strcmp(gym_members[i].username, username) == 0)
    {
      *destination = gym_members[i];
      return true;
    }
  }
  return false;
}

bool update_branch_staff(id_t id, const char full_name[], const char email[], const char phone_number[])
{
  if (is_blank_string(full_name))
  {
    LOG_ERROR("Error: Full name cannot be empty.");
    return false;
  }

  if (is_blank_string(email))
  {
    LOG_ERROR("Error: Email cannot be empty.");
    return false;
  }

  if (is_blank_string(phone_number))
  {
    LOG_ERROR("Error: Phone number cannot be empty.");
    return false;
  }

  int index = -1;
  for (int i = 0; i < branch_staff_count; i++)
  {
    if (branch_staffs[i].id == id)
    {
      index = i;
      break;
    }
  }

  if (index < 0)
  {
    LOG_ERROR("Error: No staff member found with id %lu.", (unsigned long)id);
    return false;
  }

  strcpy(branch_staffs[index].full_name, full_name);
  strcpy(branch_staffs[index].email, email);
  strcpy(branch_staffs[index].phone_number, phone_number);

  if (!rewrite_all_branch_staffs_to_file())
  {
    LOG_ERROR("Error: Failed to persist branch staff update.");
    return false;
  }

  return true;
}

bool update_gym_member(
  id_t id,
  const char full_name[],
  const char email[],
  const char phone_number[],
  const char gym_branch[],
  const char username[]
)
{
  if (is_blank_string(full_name))
  {
    LOG_ERROR("Error: Full name cannot be empty.");
    return false;
  }

  if (is_blank_string(email))
  {
    LOG_ERROR("Error: Email cannot be empty.");
    return false;
  }

  if (is_blank_string(phone_number))
  {
    LOG_ERROR("Error: Phone number cannot be empty.");
    return false;
  }

  if (is_blank_string(gym_branch))
  {
    LOG_ERROR("Error: Branch name cannot be empty.");
    return false;
  }

  if (is_blank_string(username))
  {
    LOG_ERROR("Error: Username cannot be empty.");
    return false;
  }

  int index = -1;
  for (int i = 0; i < gym_member_count; i++)
  {
    if (gym_members[i].id == id)
    {
      index = i;
      break;
    }
  }

  if (index < 0)
  {
    LOG_ERROR("Error: No gym member found with id %lu.", (unsigned long)id);
    return false;
  }

  // If username is changing, check uniqueness.
  if (strcmp(gym_members[index].username, username) != 0 && username_exists(username))
  {
    LOG_ERROR("Error: Username '%s' already exists.", username);
    return false;
  }

  strcpy(gym_members[index].full_name, full_name);
  strcpy(gym_members[index].email, email);
  strcpy(gym_members[index].phone_number, phone_number);
  strcpy(gym_members[index].gym_branch, gym_branch);
  strcpy(gym_members[index].username, username);

  if (!rewrite_all_gym_members_to_file())
  {
    LOG_ERROR("Error: Failed to persist gym member update.");
    return false;
  }

  return true;
}

bool update_gym_member_status(id_t id, membership_status_t status)
{
  int index = -1;
  for (int i = 0; i < gym_member_count; i++)
  {
    if (gym_members[i].id == id)
    {
      index = i;
      break;
    }
  }

  if (index < 0)
  {
    LOG_ERROR("Error: No gym member found with id %lu.", (unsigned long)id);
    return false;
  }

  gym_members[index].status = status;

  if (!rewrite_all_gym_members_to_file())
  {
    LOG_ERROR("Error: Failed to persist gym member status update.");
    return false;
  }

  return true;
}

bool update_gym_member_billing(id_t id, const datetime_t last_payment_date_payload, unsigned int paid_amount)
{
  int index = -1;
  for (int i = 0; i < gym_member_count; i++)
  {
    if (gym_members[i].id == id)
    {
      index = i;
      break;
    }
  }

  if (index < 0)
  {
    LOG_ERROR("Error: No gym member found with id %lu.", (unsigned long)id);
    return false;
  }

  gym_members[index].last_payment_date = last_payment_date_payload;

  // Dues never go negative; overpayments clamp at zero.
  if (gym_members[index].due_amount > paid_amount)
    gym_members[index].due_amount -= paid_amount;
  else
    gym_members[index].due_amount = 0;

  if (!rewrite_all_gym_members_to_file())
  {
    LOG_ERROR("Error: Failed to persist gym member billing update.");
    return false;
  }

  return true;
}

bool update_gym_member_lifecycle(
  id_t id,
  subscription_plan_t plan_payload,
  const datetime_t last_payment_date_payload,
  unsigned int due_amount,
  membership_status_t status
)
{
  int index = -1;
  for (int i = 0; i < gym_member_count; i++)
  {
    if (gym_members[i].id == id)
    {
      index = i;
      break;
    }
  }

  if (index < 0)
  {
    LOG_ERROR("Error: No gym member found with id %lu.", (unsigned long)id);
    return false;
  }

  gym_members[index].plan = plan_payload;
  gym_members[index].last_payment_date = last_payment_date_payload;
  gym_members[index].due_amount = due_amount;
  gym_members[index].status = status;

  if (!rewrite_all_gym_members_to_file())
  {
    LOG_ERROR("Error: Failed to persist gym member lifecycle update.");
    return false;
  }

  return true;
}

// Renames the branch reference on every staff record assigned to the old
// branch name, then rewrites the staff file from memory.
//
// Keeps no record pointing at a removed branch name. Succeeds silently when
// nothing matches, since the file is already correct.
static bool rename_branch_for_branch_staffs(const char old_branch_name[], const char new_branch_name[])
{
  bool renamed = false;
  for (int i = 0; i < branch_staff_count; i++)
  {
    if (strcmp(branch_staffs[i].gym_branch, old_branch_name) == 0)
    {
      strcpy(branch_staffs[i].gym_branch, new_branch_name);
      renamed = true;
    }
  }

  if (!renamed) return true;

  if (!rewrite_all_branch_staffs_to_file())
  {
    LOG_ERROR("Error: Failed to persist staff branch rename.");
    return false;
  }

  return true;
}

// Renames the branch reference on every member record assigned to the old
// branch name, then rewrites the members file from memory.
//
// Keeps no record pointing at a removed branch name. Succeeds silently when
// nothing matches, since the file is already correct.
static bool rename_branch_for_gym_members(const char old_branch_name[], const char new_branch_name[])
{
  bool renamed = false;
  for (int i = 0; i < gym_member_count; i++)
  {
    if (strcmp(gym_members[i].gym_branch, old_branch_name) == 0)
    {
      strcpy(gym_members[i].gym_branch, new_branch_name);
      renamed = true;
    }
  }

  if (!renamed) return true;

  if (!rewrite_all_gym_members_to_file())
  {
    LOG_ERROR("Error: Failed to persist member branch rename.");
    return false;
  }

  return true;
}

bool rename_branch_for_all_users(const char old_branch_name[], const char new_branch_name[])
{
  if (is_blank_string(old_branch_name))
  {
    LOG_ERROR("Error: Old branch name cannot be empty.");
    return false;
  }

  if (is_blank_string(new_branch_name))
  {
    LOG_ERROR("Error: New branch name cannot be empty.");
    return false;
  }

  // Cascade in two steps so a failed staff rewrite never touches members.
  if (!rename_branch_for_branch_staffs(old_branch_name, new_branch_name))
  {
    LOG_ERROR("Error: Failed to move branch staff records to '%s'.", new_branch_name);
    return false;
  }

  if (!rename_branch_for_gym_members(old_branch_name, new_branch_name))
  {
    LOG_ERROR("Error: Failed to move gym member records to '%s'.", new_branch_name);
    return false;
  }

  return true;
}

int get_gym_member_ids_by_status(membership_status_t status, id_t ids_destination[], int destination_capacity)
{
  if (ids_destination == NULL || destination_capacity <= 0) return 0;

  int copied_count = 0;
  for (int i = 0; i < gym_member_count && copied_count < destination_capacity; i++)
  {
    if (gym_members[i].status == status)
    {
      ids_destination[copied_count] = gym_members[i].id;
      copied_count++;
    }
  }

  return copied_count;
}

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "../settings.h"
#include "../types.h"
#include "../utils/datetime_utils.h"
#include "../utils/file_util.h"
#include "../utils/string_util.h"
#include "lost_found.h"
#include "user.h"

// Alias so format strings read "%lu" SEP "%s" instead of repeating FIELD_DELIMITER_STRING everywhere.
#define SEP FIELD_DELIMITER_STRING

static lost_and_found_record_t lost_and_found_records[MAX_LOST_FOUND_RECORDS];
static int lost_and_found_count;
static id_t next_lost_and_found_id;

// Formats a lost and found record as a delimiter-separated line.
static void format_lost_and_found_line(const lost_and_found_record_t record_payload, char *line_destination)
{
  snprintf(
    line_destination, LINE_BUFFER_SIZE, "%lu" SEP "%s" SEP "%s" SEP "%s" SEP "%lld" SEP "%s",
    (unsigned long)record_payload.id, record_payload.description, record_payload.reporter_username,
    record_payload.gym_branch, datetime_to_seconds(record_payload.reported_at), record_payload.resolver_username
  );
}

// Parses a pipe-delimited line into a lost and found record.
// Returns true on success, false if the field count is wrong.
static bool parse_lost_and_found_line(const char line[], lost_and_found_record_t *destination)
{
  char parts[MAX_RECORD_FIELDS][FIELD_BUFFER_SIZE];

  int count = split(line, FIELD_DELIMITER, parts, MAX_RECORD_FIELDS);
  if (count != 6) return false;

  destination->id = string_to_unsigned_long_int(parts[0]);
  strcpy(destination->description, parts[1]);
  strcpy(destination->reporter_username, parts[2]);
  strcpy(destination->gym_branch, parts[3]);
  destination->reported_at = datetime_from_seconds((long long)string_to_unsigned_long_int(parts[4]));
  strcpy(destination->resolver_username, parts[5]);
  return true;
}

// Appends a lost and found record as a delimiter-separated line to the data file.
static bool persist_lost_and_found(const lost_and_found_record_t record_payload)
{
  FILE *file = fopen(LOST_FOUND_FILE_PATH, "a");
  if (file == NULL)
  {
    LOG_ERROR("Error: Failed to open lost and found data file.");
    return false;
  }

  char line[LINE_BUFFER_SIZE];
  format_lost_and_found_line(record_payload, line);

  bool success = write_line_to_file(file, line);
  fclose(file);
  return success;
}

// Rewrites the entire lost and found file from memory after a resolution stamp.
static bool rewrite_all_lost_and_found_to_file()
{
  FILE *file = fopen(LOST_FOUND_FILE_PATH, "w");
  if (file == NULL)
  {
    LOG_ERROR("Error: Failed to open lost and found file for writing.");
    return false;
  }

  char line[LINE_BUFFER_SIZE];
  for (int i = 0; i < lost_and_found_count; i++)
  {
    format_lost_and_found_line(lost_and_found_records[i], line);
    if (!write_line_to_file(file, line))
    {
      fclose(file);
      LOG_ERROR("Error: Failed to write lost and found record.");
      return false;
    }
  }

  fclose(file);
  return true;
}

// Policy guard: the resolver must be a branch manager or the system administrator.
static bool ensure_resolver_can_resolve(const char resolver_username[])
{
  sysadmin_t sysadmin;
  if (get_sysadmin_by_username(resolver_username, &sysadmin)) return true;

  branch_staff_t staff;
  if (get_branch_staff_by_username(resolver_username, &staff) && staff.role == BRANCH_MANAGER) return true;

  return false;
}

int load_lost_and_found_records()
{
  lost_and_found_count = 0;

  FILE *file = fopen(LOST_FOUND_FILE_PATH, "r");
  if (file == NULL)
  {
    next_lost_and_found_id = 1;
    return 0;
  }

  char line[LINE_BUFFER_SIZE];
  while (lost_and_found_count < MAX_LOST_FOUND_RECORDS && read_line_from_file(file, line, LINE_BUFFER_SIZE))
  {
    if (!is_blank_string(line) && parse_lost_and_found_line(line, &lost_and_found_records[lost_and_found_count]))
      lost_and_found_count++;
  }

  fclose(file);
  next_lost_and_found_id = lost_and_found_count > 0 ? lost_and_found_records[lost_and_found_count - 1].id + 1 : 1;
  return lost_and_found_count;
}

bool report_lost_item(const char reporter_username[], const char gym_branch[], const char description[])
{
  if (is_blank_string(reporter_username))
  {
    LOG_ERROR("Error: Reporter username cannot be empty.");
    return false;
  }

  if (is_blank_string(gym_branch))
  {
    LOG_ERROR("Error: Branch name cannot be empty.");
    return false;
  }

  if (is_blank_string(description))
  {
    LOG_ERROR("Error: Item description cannot be empty.");
    return false;
  }

  if (strlen(description) >= DESCRIPTION_BUFFER_SIZE)
  {
    LOG_ERROR("Error: Item description exceeds %d characters.", DESCRIPTION_BUFFER_SIZE - 1);
    return false;
  }

  if (!username_exists(reporter_username))
  {
    LOG_ERROR("Error: No user found with username '%s'.", reporter_username);
    return false;
  }

  if (lost_and_found_count >= MAX_LOST_FOUND_RECORDS)
  {
    LOG_ERROR("Error: Maximum lost and found record count reached.");
    return false;
  }

  lost_and_found_record_t record;
  record.id = next_lost_and_found_id++;
  strcpy(record.description, description);
  strcpy(record.reporter_username, reporter_username);
  strcpy(record.gym_branch, gym_branch);
  record.reported_at = now_datetime();
  record.resolver_username[0] = '\0';

  if (!persist_lost_and_found(record))
  {
    LOG_ERROR("Error: Failed to persist lost and found record.");
    return false;
  }

  lost_and_found_records[lost_and_found_count] = record;
  lost_and_found_count++;
  return true;
}

bool resolve_lost_item(id_t record_id, const char resolver_username[])
{
  if (is_blank_string(resolver_username))
  {
    LOG_ERROR("Error: Resolver username cannot be empty.");
    return false;
  }

  int record_index = -1;
  for (int i = 0; i < lost_and_found_count; i++)
  {
    if (lost_and_found_records[i].id == record_id)
    {
      record_index = i;
      break;
    }
  }

  if (record_index < 0)
  {
    LOG_ERROR("Error: No lost and found record found with id %lu.", (unsigned long)record_id);
    return false;
  }

  if (!is_blank_string(lost_and_found_records[record_index].resolver_username))
  {
    LOG_ERROR("Error: Lost and found record %lu is already resolved.", (unsigned long)record_id);
    return false;
  }

  if (!ensure_resolver_can_resolve(resolver_username))
  {
    LOG_ERROR("Error: User '%s' is not authorized to resolve lost and found reports.", resolver_username);
    return false;
  }

  char previous_resolver[USERNAME_BUFFER_SIZE];
  strcpy(previous_resolver, lost_and_found_records[record_index].resolver_username);
  strcpy(lost_and_found_records[record_index].resolver_username, resolver_username);

  if (!rewrite_all_lost_and_found_to_file())
  {
    strcpy(lost_and_found_records[record_index].resolver_username, previous_resolver);
    LOG_ERROR("Error: Failed to persist lost and found record update.");
    return false;
  }

  return true;
}

int get_lost_and_found_for_branch(
  const char branch_name[], lost_and_found_record_t destination_records[], int destination_capacity
)
{
  if (branch_name == NULL || destination_records == NULL || destination_capacity <= 0) return 0;

  int copied_count = 0;
  for (int i = 0; i < lost_and_found_count && copied_count < destination_capacity; i++)
  {
    if (strcmp(lost_and_found_records[i].gym_branch, branch_name) == 0)
    {
      destination_records[copied_count] = lost_and_found_records[i];
      copied_count++;
    }
  }

  return copied_count;
}

int get_lost_and_found_for_reporter(
  const char reporter_username[], lost_and_found_record_t destination_records[], int destination_capacity
)
{
  if (reporter_username == NULL || destination_records == NULL || destination_capacity <= 0) return 0;

  int copied_count = 0;
  for (int i = 0; i < lost_and_found_count && copied_count < destination_capacity; i++)
  {
    if (strcmp(lost_and_found_records[i].reporter_username, reporter_username) == 0)
    {
      destination_records[copied_count] = lost_and_found_records[i];
      copied_count++;
    }
  }

  return copied_count;
}

int get_lost_and_found_for_resolver(
  const char resolver_username[], lost_and_found_record_t destination_records[], int destination_capacity
)
{
  if (resolver_username == NULL || destination_records == NULL || destination_capacity <= 0) return 0;

  int copied_count = 0;
  for (int i = 0; i < lost_and_found_count && copied_count < destination_capacity; i++)
  {
    if (strcmp(lost_and_found_records[i].resolver_username, resolver_username) == 0)
    {
      destination_records[copied_count] = lost_and_found_records[i];
      copied_count++;
    }
  }

  return copied_count;
}

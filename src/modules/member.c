#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "../settings.h"
#include "../types.h"
#include "../utils/date_util.h"
#include "../utils/file_util.h"
#include "../utils/string_util.h"
#include "member.h"
#include "user.h"

// Short alias so record format strings read as "%lu" SEP "%s" instead of
// repeating the full FIELD_DELIMITER_STRING macro at every field boundary.
#define SEP FIELD_DELIMITER_STRING

// Seconds in a single day, used to project the next due date forward.
#define SECONDS_PER_DAY 86400

static suspension_record_t suspension_records[MAX_SUSPENSION_RECORDS];
static int suspension_count;
static id_t next_suspension_id;

// Policy guard ensuring a member is still awaiting manager approval.
static bool ensure_member_is_on_hold(const gym_member_t *member_payload)
{
  return member_payload != NULL && member_payload->status == MEMBERSHIP_ON_HOLD;
}

// Policy guard ensuring a member can be suspended right now.
static bool ensure_member_is_active(const gym_member_t *member_payload)
{
  return member_payload != NULL && member_payload->status == MEMBERSHIP_ACTIVE;
}

// Policy guard ensuring a member is currently under suspension.
static bool ensure_member_is_suspended(const gym_member_t *member_payload)
{
  return member_payload != NULL && member_payload->status == MEMBERSHIP_SUSPENDED;
}

// Formats a suspension record as a delimiter-separated line.
static void format_suspension_line(const suspension_record_t *record_payload, char *line_destination)
{
  snprintf(line_destination, LINE_BUFFER_SIZE, "%lu" SEP "%lu" SEP "%s" SEP "%ld" SEP "%ld",
           (unsigned long)record_payload->id, (unsigned long)record_payload->gym_member_id, record_payload->reason,
           (long)record_payload->suspension_date, (long)record_payload->unsuspension_date);
}

// Appends a suspension record as a delimiter-separated line to the data file.
static bool persist_suspension(const suspension_record_t *record_payload)
{
  FILE *file = fopen(SUSPENSIONS_FILE_PATH, "a");
  if (file == NULL)
  {
    LOG_ERROR("Error: Failed to open suspensions data file.");
    return false;
  }

  char line[LINE_BUFFER_SIZE];
  format_suspension_line(record_payload, line);

  bool success = write_line_to_file(file, line);
  fclose(file);
  return success;
}

// Rewrites the entire suspensions file from memory.
//
// Used by unsuspend_gym_member so the persisted record reflects the
// stamped unsuspension date.
static bool rewrite_suspension_file_all()
{
  FILE *file = fopen(SUSPENSIONS_FILE_PATH, "w");
  if (file == NULL)
  {
    LOG_ERROR("Error: Failed to open suspensions file for writing.");
    return false;
  }

  char line[LINE_BUFFER_SIZE];
  for (int i = 0; i < suspension_count; i++)
  {
    format_suspension_line(&suspension_records[i], line);
    if (!write_line_to_file(file, line))
    {
      fclose(file);
      LOG_ERROR("Error: Failed to write suspension record.");
      return false;
    }
  }

  fclose(file);
  return true;
}

// Splits a pipe-delimited line into field buffers and returns the field count.
static int split_record_line(const char line[], char parts_destination[][FIELD_BUFFER_SIZE])
{
  // Map each row of parts_destination to a pointer because split() fills
  // fields through char pointers.
  char *parts[MAX_RECORD_FIELDS];
  for (int i = 0; i < MAX_RECORD_FIELDS; i++)
    parts[i] = parts_destination[i];

  return split(line, FIELD_DELIMITER, parts, MAX_RECORD_FIELDS, FIELD_BUFFER_SIZE);
}

// Parses a pipe-delimited line into a suspension record.
// Returns true on success, false if the field count is wrong.
static bool parse_suspension_line(const char line[], suspension_record_t *destination)
{
  char parts[MAX_RECORD_FIELDS][FIELD_BUFFER_SIZE];
  int count = split_record_line(line, parts);
  if (count != 5)
    return false;

  destination->id = string_to_unsigned_long_int(parts[0]);
  destination->gym_member_id = string_to_unsigned_long_int(parts[1]);
  strcpy(destination->reason, parts[2]);
  destination->suspension_date = (time_t)string_to_unsigned_long_int(parts[3]);
  destination->unsuspension_date = (time_t)string_to_unsigned_long_int(parts[4]);
  return true;
}

int load_suspensions()
{
  suspension_count = 0;

  FILE *file = fopen(SUSPENSIONS_FILE_PATH, "r");
  if (file == NULL)
  {
    next_suspension_id = 1;
    return 0;
  }

  char line[LINE_BUFFER_SIZE];
  while (suspension_count < MAX_SUSPENSION_RECORDS && read_line_from_file(file, line, LINE_BUFFER_SIZE))
  {
    if (strlen(line) > 0 && parse_suspension_line(line, &suspension_records[suspension_count]))
      suspension_count++;
  }

  fclose(file);
  next_suspension_id = suspension_count > 0 ? suspension_records[suspension_count - 1].id + 1 : 1;
  return suspension_count;
}

bool approve_gym_member(id_t member_id, time_t approval_date)
{
  gym_member_t member;
  if (!get_gym_member_by_id(member_id, &member))
  {
    LOG_ERROR("Error: No gym member found with id %lu.", (unsigned long)member_id);
    return false;
  }

  if (!ensure_member_is_on_hold(&member))
  {
    LOG_ERROR("Error: Member with id %lu is not awaiting approval.", (unsigned long)member_id);
    return false;
  }

  subscription_plan_t default_plan;
  default_plan.payable_amount = DEFAULT_PLAN_AMOUNT;
  default_plan.interval_days = DEFAULT_PLAN_INTERVAL_DAYS;

  if (!update_gym_member_lifecycle(member_id, default_plan, approval_date, default_plan.payable_amount,
                                   MEMBERSHIP_ACTIVE))
  {
    LOG_ERROR("Error: Failed to persist approval of member %lu.", (unsigned long)member_id);
    return false;
  }

  return true;
}

bool suspend_gym_member(id_t member_id, const char reason[], time_t suspension_date)
{
  if (reason == NULL || strlen(reason) == 0)
  {
    LOG_ERROR("Error: Suspension reason cannot be empty.");
    return false;
  }

  if (strlen(reason) >= REASON_BUFFER_SIZE)
  {
    LOG_ERROR("Error: Suspension reason exceeds %d characters.", REASON_BUFFER_SIZE - 1);
    return false;
  }

  gym_member_t member;
  if (!get_gym_member_by_id(member_id, &member))
  {
    LOG_ERROR("Error: No gym member found with id %lu.", (unsigned long)member_id);
    return false;
  }

  if (!ensure_member_is_active(&member))
  {
    LOG_ERROR("Error: Member with id %lu is not active and cannot be suspended.", (unsigned long)member_id);
    return false;
  }

  if (suspension_count >= MAX_SUSPENSION_RECORDS)
  {
    LOG_ERROR("Error: Maximum suspension record count reached.");
    return false;
  }

  suspension_record_t record;
  record.id = next_suspension_id++;
  record.gym_member_id = member.id;
  strcpy(record.reason, reason);
  record.suspension_date = suspension_date;
  record.unsuspension_date = 0;

  if (!persist_suspension(&record))
  {
    LOG_ERROR("Error: Failed to persist suspension record.");
    return false;
  }

  suspension_records[suspension_count] = record;
  suspension_count++;

  if (!update_gym_member_lifecycle(member_id, member.plan, member.last_payment_date, member.due_amount,
                                   MEMBERSHIP_SUSPENDED))
  {
    LOG_ERROR("Error: Failed to persist suspension of member %lu.", (unsigned long)member_id);
    return false;
  }

  return true;
}

bool unsuspend_gym_member(id_t member_id, time_t unsuspension_date)
{
  gym_member_t member;
  if (!get_gym_member_by_id(member_id, &member))
  {
    LOG_ERROR("Error: No gym member found with id %lu.", (unsigned long)member_id);
    return false;
  }

  if (!ensure_member_is_suspended(&member))
  {
    LOG_ERROR("Error: Member with id %lu is not suspended.", (unsigned long)member_id);
    return false;
  }

  if (!ensure_member_has_no_dues(&member))
  {
    LOG_ERROR("Error: Member with id %lu still owes dues and cannot be unsuspended.", (unsigned long)member_id);
    return false;
  }

  int open_index = -1;
  for (int i = 0; i < suspension_count; i++)
  {
    if (suspension_records[i].gym_member_id == member_id && suspension_records[i].unsuspension_date == 0)
    {
      open_index = i;
      break;
    }
  }

  if (open_index < 0)
  {
    LOG_ERROR("Error: No open suspension record found for member %lu.", (unsigned long)member_id);
    return false;
  }

  time_t previous_unsuspension_date = suspension_records[open_index].unsuspension_date;
  suspension_records[open_index].unsuspension_date = unsuspension_date;

  if (!rewrite_suspension_file_all())
  {
    suspension_records[open_index].unsuspension_date = previous_unsuspension_date;
    LOG_ERROR("Error: Failed to persist suspension record update.");
    return false;
  }

  if (!update_gym_member_lifecycle(member_id, member.plan, member.last_payment_date, member.due_amount,
                                   MEMBERSHIP_ACTIVE))
  {
    LOG_ERROR("Error: Failed to persist unsuspension of member %lu.", (unsigned long)member_id);
    return false;
  }

  return true;
}

int auto_suspend_overdue_members(time_t today)
{
  id_t active_ids[MAX_GYM_MEMBERS];
  int active_count = get_gym_member_ids_by_status(MEMBERSHIP_ACTIVE, active_ids, MAX_GYM_MEMBERS);

  int suspended_count = 0;
  for (int i = 0; i < active_count; i++)
  {
    gym_member_t member;
    if (!get_gym_member_by_id(active_ids[i], &member))
      continue;

    // The due date is one payment interval past the billing cycle start.
    time_t due_date = member.last_payment_date + (time_t)member.plan.interval_days * SECONDS_PER_DAY;
    if (days_between(due_date, today) < MAX_UNPAID_DAYS)
      continue;

    if (!suspend_gym_member(active_ids[i], AUTO_SUSPENSION_REASON, today))
      continue;

    suspended_count++;
  }

  return suspended_count;
}

int get_suspensions_for_member(id_t gym_member_id, suspension_record_t destination_records[], int destination_capacity)
{
  if (destination_records == NULL || destination_capacity <= 0)
    return 0;

  int copied_count = 0;
  for (int i = 0; i < suspension_count && copied_count < destination_capacity; i++)
  {
    if (suspension_records[i].gym_member_id == gym_member_id)
    {
      destination_records[copied_count] = suspension_records[i];
      copied_count++;
    }
  }

  return copied_count;
}

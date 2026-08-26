#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "../settings.h"
#include "../types.h"
#include "../utils/datetime_utils.h"
#include "../utils/file_util.h"
#include "../utils/string_util.h"
#include "payment.h"
#include "user.h"

// Alias so format strings read "%lu" SEP "%s" instead of repeating FIELD_DELIMITER_STRING everywhere.
#define SEP FIELD_DELIMITER_STRING

static payment_t payments[MAX_PAYMENT_RECORDS];
static int payment_count;
static id_t next_payment_id;

// Formats a payment as a delimiter-separated line.
static void format_payment_line(const payment_t record_payload, char *line_destination)
{
  snprintf(
    line_destination, LINE_BUFFER_SIZE, "%lu" SEP "%lu" SEP "%u" SEP "%lld" SEP "%d" SEP "%s" SEP "%d",
    (unsigned long)record_payload.id, (unsigned long)record_payload.gym_member_id, record_payload.amount,
    datetime_to_seconds(record_payload.transaction_time), (int)record_payload.transaction_type,
    record_payload.transaction_id, (int)record_payload.status
  );
}

// Parses a pipe-delimited line into a payment.
// Returns true on success, false if the field count is wrong.
static bool parse_payment_line(const char line[], payment_t *destination)
{
  char parts[MAX_RECORD_FIELDS][FIELD_BUFFER_SIZE];

  int count = split(line, FIELD_DELIMITER, parts, MAX_RECORD_FIELDS);
  if (count != 7) return false;

  destination->id = string_to_unsigned_long_int(parts[0]);
  destination->gym_member_id = string_to_unsigned_long_int(parts[1]);
  destination->amount = string_to_unsigned_int(parts[2]);
  destination->transaction_time = datetime_from_seconds((long long)string_to_unsigned_long_int(parts[3]));
  destination->transaction_type = (transaction_t)string_to_unsigned_int(parts[4]);
  strcpy(destination->transaction_id, parts[5]);
  destination->status = (payment_status_t)string_to_unsigned_int(parts[6]);
  return true;
}

// Appends a payment as a delimiter-separated line to the data file.
static bool persist_payment(const payment_t record_payload)
{
  FILE *file = fopen(PAYMENTS_FILE_PATH, "a");
  if (file == NULL)
  {
    LOG_ERROR("Error: Failed to open payments data file.");
    return false;
  }

  char line[LINE_BUFFER_SIZE];
  format_payment_line(record_payload, line);

  bool success = write_line_to_file(file, line);
  fclose(file);
  return success;
}

// Checks whether the member exists and their membership accepts payments.
//
// Active members settle running dues; suspended members clear dues blocking
// reactivation. On-hold members have no billing cycle yet; cancelled ones
// accept no money.
static bool ensure_member_can_pay(id_t gym_member_id)
{
  gym_member_t member;
  if (!get_gym_member_by_id(gym_member_id, &member)) return false;

  switch (member.status)
  {
  case MEMBERSHIP_ACTIVE:
  case MEMBERSHIP_SUSPENDED:
    return true;
  default:
    return false;
  }
}

// Persists a payment for its paying member and settles dues when completed.
//
// Appends file-first, then mirrors in memory, matching the ordering used
// across modules.
static bool settle_payment(const payment_t payment_payload)
{
  if (!persist_payment(payment_payload))
  {
    LOG_ERROR("Error: Failed to persist payment.");
    return false;
  }

  payments[payment_count] = payment_payload;
  payment_count++;

  // Gateway-reported statuses land in history only; a completed payment also
  // settles dues and restarts the billing cycle.
  if (payment_payload.status != PAYMENT_COMPLETED) return true;

  if (!update_gym_member_billing(
        payment_payload.gym_member_id, payment_payload.transaction_time, payment_payload.amount
      ))
  {
    LOG_ERROR("Error: Failed to settle dues for member %lu.", (unsigned long)payment_payload.gym_member_id);
    return false;
  }

  return true;
}

int load_payments()
{
  payment_count = 0;

  FILE *file = fopen(PAYMENTS_FILE_PATH, "r");
  if (file == NULL)
  {
    next_payment_id = 1;
    return 0;
  }

  char line[LINE_BUFFER_SIZE];
  while (payment_count < MAX_PAYMENT_RECORDS && read_line_from_file(file, line, LINE_BUFFER_SIZE))
  {
    if (strlen(line) > 0 && parse_payment_line(line, &payments[payment_count])) payment_count++;
  }

  fclose(file);
  next_payment_id = payment_count > 0 ? payments[payment_count - 1].id + 1 : 1;
  return payment_count;
}

bool record_digital_payment(const digital_payment_request_t request_payload)
{
  if (request_payload.amount == 0)
  {
    LOG_ERROR("Error: Payment amount cannot be zero.");
    return false;
  }

  if (strlen(request_payload.transaction_id) == 0)
  {
    LOG_ERROR("Error: Digital payment requires a transaction id.");
    return false;
  }

  switch (request_payload.status)
  {
  case PAYMENT_PENDING:
  case PAYMENT_COMPLETED:
  case PAYMENT_FAILED:
  case PAYMENT_INVALID:
    break;
  default:
    LOG_ERROR("Error: Unknown payment status %d.", (int)request_payload.status);
    return false;
  }

  // Only completed payments need a real timestamp; history-only attempts may go unrecorded.
  if (request_payload.status == PAYMENT_COMPLETED && is_empty_datetime(request_payload.transaction_time))
  {
    LOG_ERROR("Error: Completed payment requires a transaction time.");
    return false;
  }

  if (!ensure_member_can_pay(request_payload.gym_member_id))
  {
    LOG_ERROR("Error: Member %lu cannot receive payments.", (unsigned long)request_payload.gym_member_id);
    return false;
  }

  if (payment_count >= MAX_PAYMENT_RECORDS)
  {
    LOG_ERROR("Error: Maximum payment record count reached.");
    return false;
  }

  payment_t payment;
  payment.id = next_payment_id++;
  payment.gym_member_id = request_payload.gym_member_id;
  payment.amount = request_payload.amount;
  payment.transaction_time = request_payload.transaction_time;
  payment.transaction_type = DIGITAL_TRANSACTION;
  strcpy(payment.transaction_id, request_payload.transaction_id);
  payment.status = request_payload.status;

  return settle_payment(payment);
}

bool record_cash_payment(id_t gym_member_id, unsigned int amount)
{
  if (amount == 0)
  {
    LOG_ERROR("Error: Payment amount cannot be zero.");
    return false;
  }

  if (!ensure_member_can_pay(gym_member_id))
  {
    LOG_ERROR("Error: Member %lu cannot receive payments.", (unsigned long)gym_member_id);
    return false;
  }

  if (payment_count >= MAX_PAYMENT_RECORDS)
  {
    LOG_ERROR("Error: Maximum payment record count reached.");
    return false;
  }

  payment_t payment;
  payment.id = next_payment_id++;
  payment.gym_member_id = gym_member_id;
  payment.amount = amount;
  payment.transaction_time = now_datetime();
  payment.transaction_type = CASH_TRANSACTION;
  payment.transaction_id[0] = '\0';
  payment.status = PAYMENT_COMPLETED;

  return settle_payment(payment);
}

int get_payments_for_member(id_t gym_member_id, payment_t destination_payments[], int destination_capacity)
{
  if (destination_payments == NULL || destination_capacity <= 0) return 0;

  int count = 0;
  for (int i = 0; i < payment_count && count < destination_capacity; i++)
  {
    if (payments[i].gym_member_id == gym_member_id)
    {
      destination_payments[count] = payments[i];
      count++;
    }
  }

  return count;
}

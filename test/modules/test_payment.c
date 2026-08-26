#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "../../src/modules/payment.h"
#include "../../src/modules/user.h"
#include "../../src/settings.h"
#include "../../src/types.h"
#include "../../src/utils/datetime_utils.h"
#include "test_payment.h"

/**
 * Removes the payment and payment record data files from the test_data
 * directory. Called once at startup from test_main and between scenario resets.
 */
void cleanup_payment_files()
{
  remove(PAYMENTS_FILE_PATH);
}

/**
 * Helper: builds a datetime from explicit calendar components.
 */
static datetime_t make_datetime(int year, int month, int day, int hour, int minute, int second)
{
  datetime_t result;
  result.year = year;
  result.month = month;
  result.day = day;
  result.hour = hour;
  result.minute = minute;
  result.second = second;
  return result;
}

/**
 * Helper: asserts that actual falls inside [start_payload, end_payload].
 *
 * The wall clock keeps running while a module stamps its own now_datetime,
 * so exact equality is flaky at second precision; a bracket is not.
 */
static void assert_datetime_between(datetime_t start_payload, datetime_t actual_payload, datetime_t end_payload)
{
  assert(compare_datetime(start_payload, actual_payload) <= 0);
  assert(compare_datetime(actual_payload, end_payload) <= 0);
}

// Appends one raw line to the members data file for full record control.
static void write_member_line(const char line[])
{
  FILE *file = fopen(GYM_MEMBERS_FILE_PATH, "a");
  assert(file != NULL);
  fprintf(file, "%s\n", line);
  fclose(file);
}

// Appends a gym member record built from the fields the tests vary.
static void write_raw_member(
  id_t member_id,
  const char username[],
  datetime_t last_payment_date_payload,
  unsigned int due_amount,
  unsigned int payable_amount,
  unsigned int interval_days,
  membership_status_t status
)
{
  long long joined_seconds = datetime_to_seconds(make_datetime(2024, 1, 1, 0, 0, 0));
  char line[LINE_BUFFER_SIZE];
  snprintf(
    line, LINE_BUFFER_SIZE, "%lu|%s|%s@test.com|0181234567|Dhanmondi|%s|h|%lld|%lld|%u|%u|%u|%d",
    (unsigned long)member_id, username, username, username, joined_seconds,
    datetime_to_seconds(last_payment_date_payload), due_amount, payable_amount, interval_days, (int)status
  );
  write_member_line(line);
}

// Fills a digital payment carrier with values the tests then override.
static digital_payment_request_t make_request(id_t gym_member_id, unsigned int amount, payment_status_t status)
{
  digital_payment_request_t request;
  request.request_id = 0;
  request.gym_member_id = gym_member_id;
  request.amount = amount;
  request.transaction_time = now_datetime();
  strcpy(request.transaction_id, "TRX00000");
  request.status = status;
  return request;
}

/**
 * Helper: resets every store the payment scenarios touch, so member ids and
 * record counts stay deterministic no matter what earlier suites left behind.
 */
static void reset_stores()
{
  remove(GYM_MEMBERS_FILE_PATH);
  remove(PAYMENTS_FILE_PATH);
  assert(load_gym_members() == 0);
  assert(load_payments() == 0);
}

// ---- digital payments ----

/**
 * Verifies a completed digital payment clears dues, restarts the billing
 * cycle exactly at the reported transaction time, and persists as history.
 */
void test_record_digital_completed_payment_settles_account()
{
  reset_stores();

  write_raw_member(5, "digitalpayer", make_datetime(2024, 1, 1, 0, 0, 0), 1500, 1500, 30, MEMBERSHIP_ACTIVE);
  assert(load_gym_members() == 1);

  digital_payment_request_t request = make_request(5, 1500, PAYMENT_COMPLETED);
  request.transaction_time = make_datetime(2026, 3, 15, 10, 30, 0);
  strcpy(request.transaction_id, "TRX10001");

  assert(record_digital_payment(request) == true);

  gym_member_t found;
  assert(get_gym_member_by_id(5, &found) == true);
  assert(found.due_amount == 0);
  // Billing cycle restarts exactly at the gateway-reported time.
  assert(compare_datetime(found.last_payment_date, request.transaction_time) == 0);
  assert(found.status == MEMBERSHIP_ACTIVE);
  assert(found.plan.interval_days == 30);

  payment_t history[4];
  assert(get_payments_for_member(5, history, 4) == 1);
  assert(history[0].amount == 1500);
  assert(history[0].transaction_type == DIGITAL_TRANSACTION);
  assert(history[0].status == PAYMENT_COMPLETED);
  assert(strcmp(history[0].transaction_id, "TRX10001") == 0);
  assert(compare_datetime(history[0].transaction_time, request.transaction_time) == 0);

  // Reload both stores and confirm the settled state persisted.
  load_gym_members();
  load_payments();
  assert(get_gym_member_by_id(5, &found) == true);
  assert(found.due_amount == 0);
  assert(compare_datetime(found.last_payment_date, request.transaction_time) == 0);
  assert(get_payments_for_member(5, history, 4) == 1);
  assert(strcmp(history[0].transaction_id, "TRX10001") == 0);
}

/**
 * Verifies a partial digital payment reduces dues by exactly the paid
 * amount instead of clearing them.
 */
void test_record_partial_digital_payment_reduces_due_by_paid_amount()
{
  reset_stores();

  write_raw_member(6, "partialpay", make_datetime(2024, 2, 1, 0, 0, 0), 1500, 1500, 30, MEMBERSHIP_ACTIVE);
  assert(load_gym_members() == 1);

  digital_payment_request_t request = make_request(6, 700, PAYMENT_COMPLETED);
  strcpy(request.transaction_id, "TRX10002");

  assert(record_digital_payment(request) == true);

  gym_member_t found;
  assert(get_gym_member_by_id(6, &found) == true);
  assert(found.due_amount == 800);
  assert(compare_datetime(found.last_payment_date, request.transaction_time) == 0);
}

// ---- cash payments ----

/**
 * Verifies a trainer-recorded cash payment is trusted on handover: stamped
 * completed and dated now, reduces dues, and persists as history.
 */
void test_record_cash_payment_settles_account()
{
  reset_stores();

  write_raw_member(7, "cashpayer", make_datetime(2024, 2, 1, 0, 0, 0), 800, 1000, 30, MEMBERSHIP_ACTIVE);
  assert(load_gym_members() == 1);

  datetime_t before = now_datetime();
  assert(record_cash_payment(7, 500) == true);
  datetime_t after = now_datetime();

  gym_member_t found;
  assert(get_gym_member_by_id(7, &found) == true);
  assert(found.due_amount == 300);
  assert_datetime_between(before, found.last_payment_date, after);

  payment_t history[4];
  assert(get_payments_for_member(7, history, 4) == 1);
  assert(history[0].amount == 500);
  assert(history[0].transaction_type == CASH_TRANSACTION);
  assert(history[0].status == PAYMENT_COMPLETED);
  // Cash carries no external reference.
  assert(history[0].transaction_id[0] == '\0');

  // Reload both stores and confirm the cash payment persisted.
  datetime_t stamped = found.last_payment_date;
  load_gym_members();
  load_payments();
  assert(get_gym_member_by_id(7, &found) == true);
  assert(found.due_amount == 300);
  assert(compare_datetime(found.last_payment_date, stamped) == 0);
  assert(get_payments_for_member(7, history, 4) == 1);
  assert(history[0].transaction_type == CASH_TRANSACTION);
}

/**
 * Verifies paying more than owed clamps dues at zero, and a later payment
 * with nothing owed stays at zero.
 */
void test_payment_clamps_overpayment_to_zero_due()
{
  reset_stores();

  write_raw_member(9, "overpayer", make_datetime(2024, 3, 1, 0, 0, 0), 300, 1000, 30, MEMBERSHIP_ACTIVE);
  assert(load_gym_members() == 1);

  assert(record_cash_payment(9, 999) == true);

  gym_member_t found;
  assert(get_gym_member_by_id(9, &found) == true);
  assert(found.due_amount == 0);

  // Paying again with nothing owed never wraps around below zero.
  assert(record_cash_payment(9, 400) == true);

  assert(get_gym_member_by_id(9, &found) == true);
  assert(found.due_amount == 0);

  payment_t history[4];
  assert(get_payments_for_member(9, history, 4) == 2);
}

// ---- status handling ----

/**
 * Verifies a failed digital payment lands in the history but leaves dues
 * and the billing cycle untouched; only completed payments settle accounts.
 */
void test_non_completed_digital_payment_records_history_without_settling()
{
  reset_stores();

  datetime_t cycle_start = make_datetime(2026, 1, 10, 8, 0, 0);
  write_raw_member(11, "failedpay", cycle_start, 1200, 1200, 30, MEMBERSHIP_ACTIVE);
  assert(load_gym_members() == 1);

  digital_payment_request_t request = make_request(11, 1200, PAYMENT_FAILED);
  request.transaction_time = make_datetime(2026, 2, 1, 12, 0, 0);
  strcpy(request.transaction_id, "TRX20002");

  assert(record_digital_payment(request) == true);

  gym_member_t found;
  assert(get_gym_member_by_id(11, &found) == true);
  assert(found.due_amount == 1200);
  assert(compare_datetime(found.last_payment_date, cycle_start) == 0);

  payment_t history[4];
  assert(get_payments_for_member(11, history, 4) == 1);
  assert(history[0].status == PAYMENT_FAILED);
  assert(strcmp(history[0].transaction_id, "TRX20002") == 0);

  // The failed attempt survives reload as history only.
  load_gym_members();
  load_payments();
  assert(get_gym_member_by_id(11, &found) == true);
  assert(found.due_amount == 1200);
  assert(get_payments_for_member(11, history, 4) == 1);
  assert(history[0].status == PAYMENT_FAILED);
}

// ---- rejections ----

/**
 * Verifies both flows reject unknown members, zero amounts, on-hold and
 * cancelled memberships, missing transaction references, completed payments
 * without a timestamp, and unknown gateway statuses, leaving no trace.
 */
void test_payment_rejects_invalid_members_amounts_and_details()
{
  reset_stores();

  write_raw_member(21, "rejectactive", make_datetime(2024, 1, 1, 0, 0, 0), 900, 1000, 30, MEMBERSHIP_ACTIVE);
  write_raw_member(22, "rejectonhold", EMPTY_DATETIME, 0, 1000, 30, MEMBERSHIP_ON_HOLD);
  write_raw_member(23, "rejectcancelled", make_datetime(2024, 1, 1, 0, 0, 0), 0, 1000, 30, MEMBERSHIP_CANCELLED);
  assert(load_gym_members() == 3);

  digital_payment_request_t valid_request = make_request(21, 300, PAYMENT_COMPLETED);

  // Unknown members fail on both flows.
  valid_request.gym_member_id = 9999;
  assert(record_digital_payment(valid_request) == false);
  assert(record_cash_payment(9999, 300) == false);

  // Zero amounts fail on both flows.
  valid_request.gym_member_id = 21;
  valid_request.amount = 0;
  assert(record_digital_payment(valid_request) == false);
  assert(record_cash_payment(21, 0) == false);

  // On-hold and cancelled memberships accept no money.
  valid_request.gym_member_id = 22;
  valid_request.amount = 300;
  assert(record_digital_payment(valid_request) == false);
  assert(record_cash_payment(22, 300) == false);
  valid_request.gym_member_id = 23;
  assert(record_digital_payment(valid_request) == false);
  assert(record_cash_payment(23, 300) == false);

  // Completed digital payments need a reference and a real timestamp.
  valid_request.gym_member_id = 21;
  valid_request.transaction_id[0] = '\0';
  assert(record_digital_payment(valid_request) == false);
  strcpy(valid_request.transaction_id, "TRX30004");
  valid_request.transaction_time = EMPTY_DATETIME;
  assert(record_digital_payment(valid_request) == false);

  // Unknown gateway statuses are rejected outright.
  valid_request.transaction_time = now_datetime();
  valid_request.status = (payment_status_t)99;
  assert(record_digital_payment(valid_request) == false);

  // Nothing slipped through: no money moved, no history written.
  gym_member_t found;
  assert(get_gym_member_by_id(21, &found) == true);
  assert(found.due_amount == 900);

  payment_t history[2];
  assert(get_payments_for_member(21, history, 2) == 0);
}

// ---- payment history getter ----

/**
 * Verifies repeated payments accumulate ordered per member and that
 * capacity limits what get_payments_for_member copies.
 */
void test_get_payments_for_member_handles_history_and_capacity()
{
  reset_stores();

  write_raw_member(31, "historian", make_datetime(2024, 1, 1, 0, 0, 0), 2000, 1000, 30, MEMBERSHIP_ACTIVE);
  write_raw_member(32, "bystander", make_datetime(2024, 1, 1, 0, 0, 0), 0, 1000, 30, MEMBERSHIP_ACTIVE);
  assert(load_gym_members() == 2);

  assert(record_cash_payment(31, 1000) == true);
  assert(record_cash_payment(31, 1000) == true);
  assert(record_cash_payment(32, 100) == true);

  payment_t history[4];
  assert(get_payments_for_member(31, history, 4) == 2);
  assert(history[0].id < history[1].id);
  assert(history[0].amount == 1000);
  assert(history[1].amount == 1000);

  // A smaller destination receives only the oldest payments.
  id_t oldest_payment_id = history[0].id;
  assert(get_payments_for_member(31, history, 1) == 1);
  assert(history[0].id == oldest_payment_id);

  assert(get_payments_for_member(32, history, 4) == 1);
  assert(get_payments_for_member(9999, history, 4) == 0);
  assert(get_payments_for_member(31, NULL, 4) == 0);
  assert(get_payments_for_member(31, history, 0) == 0);
}

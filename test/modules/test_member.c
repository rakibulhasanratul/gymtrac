#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "../../src/modules/member.h"
#include "../../src/modules/user.h"
#include "../../src/settings.h"
#include "../../src/types.h"
#include "../../src/utils/datetime_utils.h"
#include "test_member.h"

/**
 * Removes the member and suspension data files from the test_data directory.
 * Called once at startup from test_main and between scenario resets.
 */
void cleanup_member_files()
{
  remove(GYM_MEMBERS_FILE_PATH);
  remove(SUSPENSIONS_FILE_PATH);
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
 * Helper: asserts actual falls inside [start_payload, end_payload].
 *
 * The clock keeps running while a module stamps now_datetime, so equality
 * flakes at second precision; a bracket does not.
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

// Creates an on-hold member through the module api and returns its id.
static id_t create_on_hold_member(const char username[])
{
  subscription_plan_t plan;
  plan.payable_amount = 1500;
  plan.interval_days = 30;

  return create_gym_member(
    "Test Member", "member@test.com", "0181234567", "Dhanmondi", username, "hashval", plan, MEMBERSHIP_ON_HOLD
  );
}

// Creates an active member through the module api and returns its id.
static id_t create_active_member(const char username[])
{
  subscription_plan_t plan;
  plan.payable_amount = 1500;
  plan.interval_days = 30;

  return create_gym_member(
    "Test Member", "member@test.com", "0181234567", "Dhanmondi", username, "hashval", plan, MEMBERSHIP_ACTIVE
  );
}

// ---- approval ----

/**
 * Verifies approving an on-hold member assigns the default plan, starts
 * billing at today's date, sets dues to the plan amount, persists.
 */
void test_approve_on_hold_member_activates_with_default_plan()
{
  cleanup_member_files();
  load_gym_members();
  load_suspensions();

  id_t member_id = create_on_hold_member("approveme");
  assert(member_id != 0);

  datetime_t before = now_datetime();
  assert(approve_gym_member(member_id) == true);
  datetime_t after = now_datetime();

  gym_member_t found;
  assert(get_gym_member_by_id(member_id, &found) == true);
  assert(found.status == MEMBERSHIP_ACTIVE);
  assert(found.plan.payable_amount == DEFAULT_PLAN_AMOUNT);
  assert(found.plan.interval_days == DEFAULT_PLAN_INTERVAL_DAYS);
  assert_datetime_between(before, found.last_payment_date, after);
  assert(found.due_amount == DEFAULT_PLAN_AMOUNT);

  // Reload and confirm the approval persisted to disk.
  datetime_t stamped = found.last_payment_date;
  load_gym_members();
  assert(get_gym_member_by_id(member_id, &found) == true);
  assert(found.status == MEMBERSHIP_ACTIVE);
  assert(found.plan.payable_amount == DEFAULT_PLAN_AMOUNT);
  assert(found.due_amount == DEFAULT_PLAN_AMOUNT);
  assert(compare_datetime(found.last_payment_date, stamped) == 0);
}

/**
 * Verifies approval rejects members that are not on hold (active or
 * suspended) and unknown ids, leaving their status untouched.
 */
void test_approve_rejects_non_on_hold_and_unknown_members()
{
  cleanup_member_files();
  write_raw_member(77, "suspme", make_datetime(2024, 1, 1, 0, 0, 0), 0, 1000, 30, MEMBERSHIP_SUSPENDED);
  assert(load_gym_members() == 1);
  load_suspensions();

  id_t active_id = create_active_member("alreadyactive");
  assert(active_id != 0);

  assert(approve_gym_member(77) == false);
  assert(approve_gym_member(active_id) == false);
  assert(approve_gym_member(9999) == false);

  gym_member_t found;
  assert(get_gym_member_by_id(77, &found) == true);
  assert(found.status == MEMBERSHIP_SUSPENDED);
  assert(get_gym_member_by_id(active_id, &found) == true);
  assert(found.status == MEMBERSHIP_ACTIVE);
}

// ---- suspension ----

/**
 * Verifies suspending an active member flips the status and writes a dated
 * suspension record with reason and a zero unsuspension date, persisted.
 */
void test_suspend_active_member_writes_dated_record()
{
  cleanup_member_files();
  load_gym_members();
  load_suspensions();

  id_t member_id = create_active_member("suspendee");
  assert(member_id != 0);

  datetime_t before = now_datetime();
  assert(suspend_gym_member(member_id, "Repeated late payments") == true);
  datetime_t after = now_datetime();

  gym_member_t found;
  assert(get_gym_member_by_id(member_id, &found) == true);
  assert(found.status == MEMBERSHIP_SUSPENDED);

  suspension_record_t records[4];
  assert(get_suspensions_for_member(member_id, records, 4) == 1);
  assert(records[0].gym_member_id == member_id);
  assert(strcmp(records[0].reason, "Repeated late payments") == 0);
  assert_datetime_between(before, records[0].suspension_date, after);
  assert(is_empty_datetime(records[0].unsuspension_date));

  // Reload both stores and confirm the suspension persisted.
  datetime_t stamped = records[0].suspension_date;
  load_gym_members();
  load_suspensions();
  assert(get_gym_member_by_id(member_id, &found) == true);
  assert(found.status == MEMBERSHIP_SUSPENDED);
  assert(get_suspensions_for_member(member_id, records, 4) == 1);
  assert(strcmp(records[0].reason, "Repeated late payments") == 0);
  assert(compare_datetime(records[0].suspension_date, stamped) == 0);
}

/**
 * Verifies suspension rejects an empty or missing reason, an unknown id,
 * an on-hold member, and an already suspended member (no double records).
 */
void test_suspend_rejects_missing_reason_and_invalid_state()
{
  cleanup_member_files();
  load_gym_members();
  load_suspensions();

  id_t on_hold_id = create_on_hold_member("holdsuspend");
  assert(on_hold_id != 0);

  assert(suspend_gym_member(on_hold_id, "") == false);
  assert(suspend_gym_member(on_hold_id, NULL) == false);
  assert(suspend_gym_member(on_hold_id, "Not active yet") == false);
  assert(suspend_gym_member(9999, "Ghost member") == false);

  id_t active_id = create_active_member("doublestop");
  assert(active_id != 0);
  assert(suspend_gym_member(active_id, "First offence") == true);
  assert(suspend_gym_member(active_id, "Second offence") == false);

  suspension_record_t records[2];
  assert(get_suspensions_for_member(active_id, records, 2) == 1);
  assert(strcmp(records[0].reason, "First offence") == 0);
  assert(get_suspensions_for_member(on_hold_id, records, 2) == 0);
}

// ---- unsuspension ----

/**
 * Verifies unsuspending reactivates the member and stamps the open record's
 * unsuspension date with today, persisted across a reload.
 */
void test_unsuspend_reactivates_member_and_closes_record()
{
  cleanup_member_files();
  load_gym_members();
  load_suspensions();

  id_t member_id = create_active_member("reinstate");
  assert(member_id != 0);

  datetime_t suspend_start = now_datetime();
  assert(suspend_gym_member(member_id, "Unpaid dues") == true);
  datetime_t suspend_end = now_datetime();
  datetime_t unsuspend_start = now_datetime();
  assert(unsuspend_gym_member(member_id) == true);
  datetime_t unsuspend_end = now_datetime();

  gym_member_t found;
  assert(get_gym_member_by_id(member_id, &found) == true);
  assert(found.status == MEMBERSHIP_ACTIVE);

  suspension_record_t records[4];
  assert(get_suspensions_for_member(member_id, records, 4) == 1);
  assert_datetime_between(suspend_start, records[0].suspension_date, suspend_end);
  assert_datetime_between(unsuspend_start, records[0].unsuspension_date, unsuspend_end);

  // Reload both stores and confirm the closed record persisted.
  suspension_record_t stamped = records[0];
  load_gym_members();
  load_suspensions();
  assert(get_gym_member_by_id(member_id, &found) == true);
  assert(found.status == MEMBERSHIP_ACTIVE);
  assert(get_suspensions_for_member(member_id, records, 4) == 1);
  assert(compare_datetime(records[0].suspension_date, stamped.suspension_date) == 0);
  assert(compare_datetime(records[0].unsuspension_date, stamped.unsuspension_date) == 0);
}

/**
 * Verifies unsuspension rejects a member with outstanding dues (the record
 * stays open), a non-suspended member, and an unknown id.
 */
void test_unsuspend_rejects_indebted_and_invalid_members()
{
  cleanup_member_files();
  // Indebted suspended member written raw because creation starts at zero dues.
  write_raw_member(5, "debtor", make_datetime(2024, 1, 1, 0, 0, 0), 900, 1000, 30, MEMBERSHIP_SUSPENDED);
  assert(load_gym_members() == 1);

  // Open suspension record for member 5, written directly into the store file.
  FILE *file = fopen(SUSPENSIONS_FILE_PATH, "w");
  assert(file != NULL);
  fprintf(
    file, "1|5|Unpaid dues|%lld|%lld\n", datetime_to_seconds(make_datetime(2026, 4, 1, 0, 0, 0)),
    datetime_to_seconds(EMPTY_DATETIME)
  );
  fclose(file);
  assert(load_suspensions() == 1);

  assert(unsuspend_gym_member(5) == false);

  gym_member_t found;
  assert(get_gym_member_by_id(5, &found) == true);
  assert(found.status == MEMBERSHIP_SUSPENDED);

  suspension_record_t records[2];
  assert(get_suspensions_for_member(5, records, 2) == 1);
  assert(is_empty_datetime(records[0].unsuspension_date));

  id_t active_id = create_active_member("notsuspended");
  assert(active_id != 0);
  assert(unsuspend_gym_member(active_id) == false);
  assert(unsuspend_gym_member(9999) == false);
}

// ---- auto-suspend sweep ----

/**
 * Verifies the sweep suspends only active members at or beyond the grace
 * limit (90+ days past due), writing auto reasons dated today, and leaves
 * members inside the window plus on-hold and already suspended ones alone.
 */
void test_auto_suspend_sweeps_only_overdue_active_members()
{
  cleanup_member_files();

  datetime_t today = now_datetime();

  // Due 100 days ago: beyond the grace period.
  write_raw_member(1, "overdue100", add_days(today, -(30 + 100)), 1000, 1000, 30, MEMBERSHIP_ACTIVE);
  // Due 89 days ago: still inside the grace period.
  write_raw_member(2, "overdue89", add_days(today, -(30 + 89)), 500, 1000, 30, MEMBERSHIP_ACTIVE);
  // Due exactly 90 days ago: right on the grace limit.
  write_raw_member(3, "overdue90", add_days(today, -(30 + 90)), 1000, 1000, 30, MEMBERSHIP_ACTIVE);
  // On-hold and already suspended members never sweep regardless of age.
  write_raw_member(4, "onhold", EMPTY_DATETIME, 0, 1000, 30, MEMBERSHIP_ON_HOLD);
  write_raw_member(5, "alreadysusp", add_days(today, -(30 + 200)), 700, 1000, 30, MEMBERSHIP_SUSPENDED);

  assert(load_gym_members() == 5);
  load_suspensions();

  datetime_t before = now_datetime();
  assert(auto_suspend_overdue_members() == 2);
  datetime_t after = now_datetime();

  gym_member_t found;
  assert(get_gym_member_by_id(1, &found) == true);
  assert(found.status == MEMBERSHIP_SUSPENDED);
  assert(get_gym_member_by_id(3, &found) == true);
  assert(found.status == MEMBERSHIP_SUSPENDED);
  assert(get_gym_member_by_id(2, &found) == true);
  assert(found.status == MEMBERSHIP_ACTIVE);
  assert(get_gym_member_by_id(4, &found) == true);
  assert(found.status == MEMBERSHIP_ON_HOLD);
  assert(get_gym_member_by_id(5, &found) == true);
  assert(found.status == MEMBERSHIP_SUSPENDED);

  suspension_record_t records[4];
  assert(get_suspensions_for_member(1, records, 4) == 1);
  assert(strcmp(records[0].reason, AUTO_SUSPENSION_REASON) == 0);
  assert_datetime_between(before, records[0].suspension_date, after);
  assert(get_suspensions_for_member(3, records, 4) == 1);
  assert(strcmp(records[0].reason, AUTO_SUSPENSION_REASON) == 0);
  assert_datetime_between(before, records[0].suspension_date, after);
  assert(get_suspensions_for_member(2, records, 4) == 0);
  assert(get_suspensions_for_member(4, records, 4) == 0);
  assert(get_suspensions_for_member(5, records, 4) == 0);

  // The sweep persists; reloading shows the same state.
  load_gym_members();
  load_suspensions();
  assert(get_gym_member_by_id(1, &found) == true);
  assert(found.status == MEMBERSHIP_SUSPENDED);
  assert(get_suspensions_for_member(1, records, 4) == 1);
  assert(get_suspensions_for_member(2, records, 4) == 0);
}

/**
 * Verifies the sweep reports zero and changes nothing when every active
 * member paid recently.
 */
void test_auto_suspend_returns_zero_when_nobody_overdue()
{
  cleanup_member_files();

  datetime_t today = now_datetime();
  write_raw_member(1, "freshpayer", add_days(today, -10), 1000, 1000, 30, MEMBERSHIP_ACTIVE);
  assert(load_gym_members() == 1);
  load_suspensions();

  assert(auto_suspend_overdue_members() == 0);

  gym_member_t found;
  assert(get_gym_member_by_id(1, &found) == true);
  assert(found.status == MEMBERSHIP_ACTIVE);

  suspension_record_t records[2];
  assert(get_suspensions_for_member(1, records, 2) == 0);
}

/**
 * Verifies repeated suspend/unsuspend cycles accumulate ordered records per
 * member and that capacity limits what get_suspensions_for_member copies.
 */
void test_get_suspensions_for_member_handles_history_and_capacity()
{
  cleanup_member_files();
  load_gym_members();
  load_suspensions();

  id_t member_id = create_active_member("twicestop");
  assert(member_id != 0);

  datetime_t first_suspend_start = now_datetime();
  assert(suspend_gym_member(member_id, "Late fees") == true);
  datetime_t first_suspend_end = now_datetime();
  datetime_t unsuspend_start = now_datetime();
  assert(unsuspend_gym_member(member_id) == true);
  datetime_t unsuspend_end = now_datetime();
  assert(suspend_gym_member(member_id, "Again late") == true);

  suspension_record_t records[2];
  assert(get_suspensions_for_member(member_id, records, 2) == 2);
  assert(records[0].id < records[1].id);
  assert(strcmp(records[0].reason, "Late fees") == 0);
  assert_datetime_between(first_suspend_start, records[0].suspension_date, first_suspend_end);
  assert_datetime_between(unsuspend_start, records[0].unsuspension_date, unsuspend_end);
  assert(strcmp(records[1].reason, "Again late") == 0);
  assert(is_empty_datetime(records[1].unsuspension_date));

  // A smaller destination only receives the oldest records.
  assert(get_suspensions_for_member(member_id, records, 1) == 1);
  assert(strcmp(records[0].reason, "Late fees") == 0);

  assert(get_suspensions_for_member(9999, records, 2) == 0);
  assert(get_suspensions_for_member(member_id, records, 0) == 0);
}

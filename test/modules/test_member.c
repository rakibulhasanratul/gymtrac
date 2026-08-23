#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "../../src/modules/member.h"
#include "../../src/modules/user.h"
#include "../../src/settings.h"
#include "../../src/types.h"
#include "../../src/utils/date_util.h"
#include "test_member.h"

// Seconds in a single day, used to backdate payment dates in raw records.
#define SECONDS_PER_DAY 86400

/**
 * Removes the member and suspension data files from the test_data directory.
 * Called once at startup from test_main and between scenario resets.
 */
void cleanup_member_files()
{
  remove(GYM_MEMBERS_FILE_PATH);
  remove(SUSPENSIONS_FILE_PATH);
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
  time_t last_payment_date,
  unsigned int due_amount,
  unsigned int payable_amount,
  unsigned int interval_days,
  membership_status_t status
)
{
  char line[LINE_BUFFER_SIZE];
  snprintf(
    line, LINE_BUFFER_SIZE, "%lu|%s|%s@test.com|0181234567|Dhanmondi|%s|h|1704067200|%ld|%u|%u|%u|%d",
    (unsigned long)member_id, username, username, username, (long)last_payment_date, due_amount, payable_amount,
    interval_days, (int)status
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

  time_t approval_date = get_today();
  assert(approve_gym_member(member_id) == true);

  gym_member_t found;
  assert(get_gym_member_by_id(member_id, &found) == true);
  assert(found.status == MEMBERSHIP_ACTIVE);
  assert(found.plan.payable_amount == DEFAULT_PLAN_AMOUNT);
  assert(found.plan.interval_days == DEFAULT_PLAN_INTERVAL_DAYS);
  assert(found.last_payment_date == approval_date);
  assert(found.due_amount == DEFAULT_PLAN_AMOUNT);

  // Reload and confirm the approval persisted to disk.
  load_gym_members();
  assert(get_gym_member_by_id(member_id, &found) == true);
  assert(found.status == MEMBERSHIP_ACTIVE);
  assert(found.plan.payable_amount == DEFAULT_PLAN_AMOUNT);
  assert(found.last_payment_date == approval_date);
  assert(found.due_amount == DEFAULT_PLAN_AMOUNT);
}

/**
 * Verifies approval rejects members that are not on hold (active or
 * suspended) and unknown ids, leaving their status untouched.
 */
void test_approve_rejects_non_on_hold_and_unknown_members()
{
  cleanup_member_files();
  write_raw_member(77, "suspme", 1704067200, 0, 1000, 30, MEMBERSHIP_SUSPENDED);
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

  assert(suspend_gym_member(member_id, "Repeated late payments") == true);

  gym_member_t found;
  assert(get_gym_member_by_id(member_id, &found) == true);
  assert(found.status == MEMBERSHIP_SUSPENDED);

  suspension_record_t records[4];
  assert(get_suspensions_for_member(member_id, records, 4) == 1);
  assert(records[0].gym_member_id == member_id);
  assert(strcmp(records[0].reason, "Repeated late payments") == 0);
  assert(records[0].suspension_date == get_today());
  assert(records[0].unsuspension_date == 0);

  // Reload both stores and confirm the suspension persisted.
  load_gym_members();
  load_suspensions();
  assert(get_gym_member_by_id(member_id, &found) == true);
  assert(found.status == MEMBERSHIP_SUSPENDED);
  assert(get_suspensions_for_member(member_id, records, 4) == 1);
  assert(strcmp(records[0].reason, "Repeated late payments") == 0);
  assert(records[0].suspension_date == get_today());
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

  time_t suspension_date = get_today();
  assert(suspend_gym_member(member_id, "Unpaid dues") == true);
  assert(unsuspend_gym_member(member_id) == true);

  gym_member_t found;
  assert(get_gym_member_by_id(member_id, &found) == true);
  assert(found.status == MEMBERSHIP_ACTIVE);

  suspension_record_t records[4];
  assert(get_suspensions_for_member(member_id, records, 4) == 1);
  assert(records[0].suspension_date == suspension_date);
  assert(records[0].unsuspension_date == suspension_date);

  // Reload both stores and confirm the closed record persisted.
  load_gym_members();
  load_suspensions();
  assert(get_gym_member_by_id(member_id, &found) == true);
  assert(found.status == MEMBERSHIP_ACTIVE);
  assert(get_suspensions_for_member(member_id, records, 4) == 1);
  assert(records[0].suspension_date == suspension_date);
  assert(records[0].unsuspension_date == suspension_date);
}

/**
 * Verifies unsuspension rejects a member with outstanding dues (the record
 * stays open), a non-suspended member, and an unknown id.
 */
void test_unsuspend_rejects_indebted_and_invalid_members()
{
  cleanup_member_files();
  // Indebted suspended member written raw because creation starts at zero dues.
  write_raw_member(5, "debtor", 1704067200, 900, 1000, 30, MEMBERSHIP_SUSPENDED);
  assert(load_gym_members() == 1);

  // Open suspension record for member 5, written directly into the store file.
  FILE *file = fopen(SUSPENSIONS_FILE_PATH, "w");
  assert(file != NULL);
  fprintf(file, "1|5|Unpaid dues|%ld|0\n", (long)string_to_time_t("2026-04-01"));
  fclose(file);
  assert(load_suspensions() == 1);

  assert(unsuspend_gym_member(5) == false);

  gym_member_t found;
  assert(get_gym_member_by_id(5, &found) == true);
  assert(found.status == MEMBERSHIP_SUSPENDED);

  suspension_record_t records[2];
  assert(get_suspensions_for_member(5, records, 2) == 1);
  assert(records[0].unsuspension_date == 0);

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

  time_t today = get_today();

  // Due 100 days ago: beyond the grace period.
  write_raw_member(1, "overdue100", today - (time_t)((30 + 100) * SECONDS_PER_DAY), 1000, 1000, 30, MEMBERSHIP_ACTIVE);
  // Due 89 days ago: still inside the grace period.
  write_raw_member(2, "overdue89", today - (time_t)((30 + 89) * SECONDS_PER_DAY), 500, 1000, 30, MEMBERSHIP_ACTIVE);
  // Due exactly 90 days ago: right on the grace limit.
  write_raw_member(3, "overdue90", today - (time_t)((30 + 90) * SECONDS_PER_DAY), 1000, 1000, 30, MEMBERSHIP_ACTIVE);
  // On-hold and already suspended members never sweep regardless of age.
  write_raw_member(4, "onhold", 0, 0, 1000, 30, MEMBERSHIP_ON_HOLD);
  write_raw_member(
    5, "alreadysusp", today - (time_t)((30 + 200) * SECONDS_PER_DAY), 700, 1000, 30, MEMBERSHIP_SUSPENDED
  );

  assert(load_gym_members() == 5);
  load_suspensions();

  assert(auto_suspend_overdue_members() == 2);

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
  assert(records[0].suspension_date == today);
  assert(get_suspensions_for_member(3, records, 4) == 1);
  assert(strcmp(records[0].reason, AUTO_SUSPENSION_REASON) == 0);
  assert(records[0].suspension_date == today);
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

  time_t today = get_today();
  write_raw_member(1, "freshpayer", today - (time_t)(10 * SECONDS_PER_DAY), 1000, 1000, 30, MEMBERSHIP_ACTIVE);
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

  time_t suspension_date = get_today();
  assert(suspend_gym_member(member_id, "Late fees") == true);
  assert(unsuspend_gym_member(member_id) == true);
  assert(suspend_gym_member(member_id, "Again late") == true);

  suspension_record_t records[2];
  assert(get_suspensions_for_member(member_id, records, 2) == 2);
  assert(records[0].id < records[1].id);
  assert(strcmp(records[0].reason, "Late fees") == 0);
  assert(records[0].suspension_date == suspension_date);
  assert(records[0].unsuspension_date == suspension_date);
  assert(strcmp(records[1].reason, "Again late") == 0);
  assert(records[1].unsuspension_date == 0);

  // A smaller destination only receives the oldest records.
  assert(get_suspensions_for_member(member_id, records, 1) == 1);
  assert(strcmp(records[0].reason, "Late fees") == 0);

  assert(get_suspensions_for_member(9999, records, 2) == 0);
  assert(get_suspensions_for_member(member_id, records, 0) == 0);
}

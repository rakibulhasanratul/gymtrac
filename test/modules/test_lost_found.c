#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "../../src/modules/lost_found.h"
#include "../../src/modules/user.h"
#include "../../src/settings.h"
#include "../../src/types.h"
#include "../../src/utils/datetime_utils.h"
#include "test_lost_found.h"

/**
 * Removes the lost and found data files from the test_data directory.
 * Called once at startup from test_main and between scenario resets.
 */
void cleanup_lost_found_files()
{
  remove(LOST_FOUND_FILE_PATH);
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

// Creates a gym member through the module api and returns its id.
static id_t create_member(const char username[], const char gym_branch[])
{
  char email[FULL_NAME_BUFFER_SIZE];
  snprintf(email, FULL_NAME_BUFFER_SIZE, "%s@test.com", username);

  subscription_plan_t plan = {.payable_amount = 1000, .interval_days = 30};
  return create_gym_member(username, email, "0181234567", gym_branch, username, "h", plan, MEMBERSHIP_ACTIVE);
}

// Creates a branch staff member through the module api and returns its id.
static id_t create_staff(const char username[], const char gym_branch[], staff_role_t role)
{
  char email[FULL_NAME_BUFFER_SIZE];
  snprintf(email, FULL_NAME_BUFFER_SIZE, "%s@test.com", username);

  return create_branch_staff(username, email, "0181234567", gym_branch, username, "h", role);
}

// Creates a sysadmin through the module api and returns its id.
static id_t create_sysadmin_user(const char username[])
{
  return create_sysadmin(username, "h");
}

/**
 * Helper: resets every store lost and found scenarios touch, keeping ids and
 * counts deterministic regardless of what earlier suites left behind.
 */
static void reset_stores()
{
  remove(GYM_MEMBERS_FILE_PATH);
  remove(BRANCH_STAFF_FILE_PATH);
  remove(SYSADMINS_FILE_PATH);
  remove(LOST_FOUND_FILE_PATH);
  assert(load_gym_members() == 0);
  assert(load_branch_staff() == 0);
  assert(load_sysadmins() == 0);
  assert(load_lost_and_found_records() == 0);
}

// ---- reporting ----

/**
 * Verifies an item report stores the description, reporter, branch snapshot,
 * and reported-at timestamp, leaves the record open, and survives a reload.
 */
void test_report_item_persists_with_branch_snapshot_and_open_state()
{
  reset_stores();

  assert(create_member("reporter", "Dhanmondi") != 0);

  datetime_t before = now_datetime();
  assert(report_lost_item("reporter", "Dhanmondi", "Black water bottle") == true);
  datetime_t after = now_datetime();

  lost_and_found_record_t records[4];
  assert(get_lost_and_found_for_reporter("reporter", records, 4) == 1);
  assert(strcmp(records[0].description, "Black water bottle") == 0);
  assert(strcmp(records[0].reporter_username, "reporter") == 0);
  assert(strcmp(records[0].gym_branch, "Dhanmondi") == 0);
  // An open record carries no resolver yet.
  assert(strlen(records[0].resolver_username) == 0);
  assert_datetime_between(before, records[0].reported_at, after);

  // Reload both stores and confirm the report persisted.
  load_gym_members();
  load_lost_and_found_records();
  assert(get_lost_and_found_for_reporter("reporter", records, 4) == 1);
  assert(strcmp(records[0].description, "Black water bottle") == 0);
  assert(strcmp(records[0].gym_branch, "Dhanmondi") == 0);
  assert(strlen(records[0].resolver_username) == 0);
}

/**
 * Verifies members, branch staff, and the system administrator can all report
 * lost or found items.
 */
void test_report_works_for_any_role()
{
  reset_stores();

  assert(create_member("memreporter", "Dhanmondi") != 0);
  assert(create_staff("staffreporter", "Uttara", TRAINER) != 0);
  assert(create_sysadmin_user("adminreporter") != 0);

  // Member reports against their branch.
  assert(report_lost_item("memreporter", "Dhanmondi", "Member item") == true);
  // Staff reports against their branch.
  assert(report_lost_item("staffreporter", "Uttara", "Staff item") == true);
  // System administrator reports against any branch.
  assert(report_lost_item("adminreporter", "Dhanmondi", "Owner item") == true);

  lost_and_found_record_t records[4];
  assert(get_lost_and_found_for_branch("Dhanmondi", records, 4) == 2);
  assert(get_lost_and_found_for_branch("Uttara", records, 4) == 1);

  assert(get_lost_and_found_for_reporter("memreporter", records, 4) == 1);
  assert(get_lost_and_found_for_reporter("staffreporter", records, 4) == 1);
  assert(get_lost_and_found_for_reporter("adminreporter", records, 4) == 1);
}

/**
 * Verifies branch listing only returns reports whose snapshotted branch
 * matches, reporter listing only own reports, resolver listing only records
 * they resolved, capacity limits copies, and invalid arguments return zero.
 */
void test_get_lost_and_found_scopes_by_branch_reporter_and_resolver()
{
  reset_stores();

  assert(create_member("dhanmember", "Dhanmondi") != 0);
  assert(create_member("uttaramember", "Uttara") != 0);
  assert(create_staff("manager", "Dhanmondi", BRANCH_MANAGER) != 0);

  assert(report_lost_item("dhanmember", "Dhanmondi", "Dhanmondi item one") == true);
  assert(report_lost_item("dhanmember", "Dhanmondi", "Dhanmondi item two") == true);
  assert(report_lost_item("uttaramember", "Uttara", "Uttara item one") == true);

  // Resolve one record so the resolver getter has something to find.
  lost_and_found_record_t records[4];
  assert(get_lost_and_found_for_reporter("dhanmember", records, 4) == 2);
  assert(resolve_lost_item(records[0].id, "manager") == true);

  // Branch scoping via snapshot.
  assert(get_lost_and_found_for_branch("Dhanmondi", records, 4) == 2);
  assert(get_lost_and_found_for_branch("Uttara", records, 4) == 1);
  assert(strcmp(records[0].description, "Uttara item one") == 0);
  // A branch with no reports sees nothing; case differences match nothing.
  assert(get_lost_and_found_for_branch("Mirpur", records, 4) == 0);
  assert(get_lost_and_found_for_branch("dhanmondi", records, 4) == 0);

  // Reporter scoping.
  assert(get_lost_and_found_for_reporter("dhanmember", records, 4) == 2);
  assert(strcmp(records[0].description, "Dhanmondi item one") == 0);
  assert(strcmp(records[1].description, "Dhanmondi item two") == 0);
  assert(get_lost_and_found_for_reporter("nobody", records, 4) == 0);

  // Resolver scoping.
  assert(get_lost_and_found_for_resolver("manager", records, 4) == 1);
  assert(strcmp(records[0].description, "Dhanmondi item one") == 0);
  assert(get_lost_and_found_for_resolver("nobody", records, 4) == 0);

  // A smaller destination receives only the oldest reports.
  assert(get_lost_and_found_for_reporter("dhanmember", records, 1) == 1);
  assert(strcmp(records[0].description, "Dhanmondi item one") == 0);

  // Invalid arguments.
  assert(get_lost_and_found_for_branch(NULL, records, 4) == 0);
  assert(get_lost_and_found_for_branch("Dhanmondi", NULL, 4) == 0);
  assert(get_lost_and_found_for_branch("Dhanmondi", records, 0) == 0);
  assert(get_lost_and_found_for_reporter(NULL, records, 4) == 0);
  assert(get_lost_and_found_for_resolver(NULL, records, 4) == 0);
  assert(get_lost_and_found_for_resolver("manager", NULL, 4) == 0);
  assert(get_lost_and_found_for_resolver("manager", records, 0) == 0);
}

// ---- resolution ----

/**
 * Verifies branch managers and the system administrator can resolve reports.
 * Resolving stamps the resolver username on the record, which survives reload.
 * A second resolution of the same record is rejected.
 */
void test_resolve_lost_item_by_manager_and_sysadmin()
{
  reset_stores();

  assert(create_member("targetreporter", "Dhanmondi") != 0);
  id_t manager_id = create_staff("manager", "Dhanmondi", BRANCH_MANAGER);
  assert(manager_id != 0);
  assert(create_sysadmin_user("sysadmin") != 0);

  assert(report_lost_item("targetreporter", "Dhanmondi", "Lost gym card") == true);
  assert(report_lost_item("targetreporter", "Dhanmondi", "Found keys") == true);

  lost_and_found_record_t records[4];
  assert(get_lost_and_found_for_reporter("targetreporter", records, 4) == 2);
  id_t first_record_id = records[0].id;
  id_t second_record_id = records[1].id;

  // Manager resolves one.
  assert(resolve_lost_item(first_record_id, "manager") == true);
  // System administrator resolves the other.
  assert(resolve_lost_item(second_record_id, "sysadmin") == true);

  // A second resolution is rejected.
  assert(resolve_lost_item(first_record_id, "sysadmin") == false);

  assert(get_lost_and_found_for_reporter("targetreporter", records, 4) == 2);
  assert(strcmp(records[0].resolver_username, "manager") == 0);
  assert(strcmp(records[1].resolver_username, "sysadmin") == 0);

  // Reload every store and confirm the resolutions persisted.
  load_gym_members();
  load_branch_staff();
  load_sysadmins();
  load_lost_and_found_records();
  assert(get_lost_and_found_for_reporter("targetreporter", records, 4) == 2);
  assert(strcmp(records[0].resolver_username, "manager") == 0);
  assert(strcmp(records[1].resolver_username, "sysadmin") == 0);
}

// ---- rejections ----

/**
 * Verifies reporting rejects empty or oversized descriptions, empty branch
 * names, and usernames that belong to no user, leaving no record behind.
 */
void test_report_rejects_invalid_input()
{
  reset_stores();

  assert(create_member("pickyreporter", "Dhanmondi") != 0);

  // Unknown reporters are rejected outright.
  assert(report_lost_item("ghost", "Dhanmondi", "Some item") == false);
  assert(report_lost_item("", "Dhanmondi", "Some item") == false);
  assert(report_lost_item(NULL, "Dhanmondi", "Some item") == false);

  // Branch name must be non-empty.
  assert(report_lost_item("pickyreporter", "", "Some item") == false);
  assert(report_lost_item("pickyreporter", NULL, "Some item") == false);

  // Descriptions must be non-empty and fit the field buffer.
  assert(report_lost_item("pickyreporter", "Dhanmondi", "") == false);
  assert(report_lost_item("pickyreporter", "Dhanmondi", NULL) == false);

  char oversized_description[DESCRIPTION_BUFFER_SIZE + 1];
  memset(oversized_description, 'x', DESCRIPTION_BUFFER_SIZE);
  oversized_description[DESCRIPTION_BUFFER_SIZE] = '\0';
  assert(report_lost_item("pickyreporter", "Dhanmondi", oversized_description) == false);

  // Nothing slipped through: no report was written.
  lost_and_found_record_t records[2];
  assert(get_lost_and_found_for_reporter("pickyreporter", records, 2) == 0);
}

/**
 * Verifies resolving rejects unknown record ids, empty usernames, and
 * unauthorized users (trainers and members cannot resolve), leaving the
 * record open.
 */
void test_resolve_rejects_invalid_input()
{
  reset_stores();

  assert(create_member("memreporter", "Uttara") != 0);
  assert(create_staff("manager", "Uttara", BRANCH_MANAGER) != 0);
  assert(create_staff("trainer", "Uttara", TRAINER) != 0);
  assert(create_member("plainmember", "Uttara") != 0);

  assert(report_lost_item("memreporter", "Uttara", "Mystery item") == true);

  lost_and_found_record_t records[2];
  assert(get_lost_and_found_for_reporter("memreporter", records, 2) == 1);
  id_t record_id = records[0].id;

  // Unknown record ids fail.
  assert(resolve_lost_item(9999, "manager") == false);

  // Empty usernames fail.
  assert(resolve_lost_item(record_id, "") == false);
  assert(resolve_lost_item(record_id, NULL) == false);

  // Unknown usernames fail.
  assert(resolve_lost_item(record_id, "ghost") == false);

  // Trainers cannot resolve.
  assert(resolve_lost_item(record_id, "trainer") == false);

  // Members cannot resolve.
  assert(resolve_lost_item(record_id, "plainmember") == false);

  // The record stays open after every rejection.
  load_lost_and_found_records();
  assert(get_lost_and_found_for_reporter("memreporter", records, 2) == 1);
  assert(strlen(records[0].resolver_username) == 0);
}

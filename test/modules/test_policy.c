#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "../../src/modules/branch.h"
#include "../../src/modules/lost_found.h"
#include "../../src/modules/policy.h"
#include "../../src/modules/session.h"
#include "../../src/modules/user.h"
#include "../../src/settings.h"
#include "../../src/types.h"
#include "../../src/utils/datetime_utils.h"
#include "test_policy.h"

// Two branches, each with one gym member and one branch staff. Used to
// exercise cross-branch policy checks without rebuilding fixtures per test.
static const char *BRANCH_A = "Dhanmondi";
static const char *BRANCH_B = "Gulshan";
static const char *MEMBER_A = "alice";
static const char *MEMBER_B = "bob";
static const char *STAFF_A = "managerA";
static const char *STAFF_B = "managerB";

// Writes a gym member record with a chosen due_amount to the data file.
// Used to test the zero-dues guard in ensure_member_deletion_is_allowed.
static void write_raw_member_with_dues(
  id_t member_id, const char username[], unsigned int due_amount, membership_status_t status
)
{
  FILE *file = fopen(GYM_MEMBERS_FILE_PATH, "a");
  assert(file != NULL);
  long long joined_seconds =
    datetime_to_seconds((datetime_t){.year = 2024, .month = 1, .day = 1, .hour = 0, .minute = 0, .second = 0});
  fprintf(
    file, "%lu|%s|%s@test.com|0181234567|%s|%s|h|%lld|%lld|%u|%u|%u|%d\n", (unsigned long)member_id, username, username,
    BRANCH_A, username, joined_seconds, joined_seconds, due_amount, 1000u, 30u, (int)status
  );
  fclose(file);
}

// Resets the in-memory stores and seeds the standard fixture set.
static void seed_fixture()
{
  remove(GYM_BRANCHES_FILE_PATH);
  remove(BRANCH_STAFF_FILE_PATH);
  remove(GYM_MEMBERS_FILE_PATH);
  remove(LOST_FOUND_FILE_PATH);

  load_branches();
  add_branch(BRANCH_A);
  add_branch(BRANCH_B);

  load_branch_staff();
  load_gym_members();
  load_lost_and_found_records();

  // Branch A: one manager, one member.
  id_t manager_a =
    create_branch_staff("Manager A", "mgrA@test.com", "01811111111", BRANCH_A, STAFF_A, "h", BRANCH_MANAGER);
  assert(manager_a != 0);
  id_t member_a = create_gym_member(
    "Alice", "alice@test.com", "01811111112", BRANCH_A, MEMBER_A, "h",
    (subscription_plan_t){.payable_amount = 1000, .interval_days = 30}, MEMBERSHIP_ACTIVE
  );
  assert(member_a != 0);

  // Branch B: one manager, one member.
  id_t manager_b =
    create_branch_staff("Manager B", "mgrB@test.com", "01822222221", BRANCH_B, STAFF_B, "h", BRANCH_MANAGER);
  assert(manager_b != 0);
  id_t member_b = create_gym_member(
    "Bob", "bob@test.com", "01822222222", BRANCH_B, MEMBER_B, "h",
    (subscription_plan_t){.payable_amount = 1000, .interval_days = 30}, MEMBERSHIP_ACTIVE
  );
  assert(member_b != 0);
}

// Looks up a gym member by username and returns the id.
static id_t find_member_id(const char username[])
{
  gym_member_t m;
  if (!get_gym_member_by_username(username, &m)) return 0;
  return m.id;
}

// Looks up a branch staff by username and returns the id.
static id_t find_staff_id(const char username[])
{
  branch_staff_t s;
  if (!get_branch_staff_by_username(username, &s)) return 0;
  return s.id;
}

// ---- membership approval ----

void test_membership_approval_sysadmin_allows_any()
{
  seed_fixture();
  id_t member_b = find_member_id(MEMBER_B);

  set_session_context(USER_ROLE_SYSADMIN, 1, "admin", "");
  assert(ensure_membership_approval_is_allowed(member_b) == true);
  clear_session_context();
}

void test_membership_approval_manager_own_branch_allows()
{
  seed_fixture();
  id_t member_a = find_member_id(MEMBER_A);
  id_t manager_a = find_staff_id(STAFF_A);

  set_session_context(USER_ROLE_BRANCH_MANAGER, manager_a, STAFF_A, BRANCH_A);
  assert(ensure_membership_approval_is_allowed(member_a) == true);
  clear_session_context();
}

void test_membership_approval_manager_other_branch_denies()
{
  seed_fixture();
  id_t member_b = find_member_id(MEMBER_B);
  id_t manager_a = find_staff_id(STAFF_A);

  set_session_context(USER_ROLE_BRANCH_MANAGER, manager_a, STAFF_A, BRANCH_A);
  assert(ensure_membership_approval_is_allowed(member_b) == false);
  clear_session_context();
}

void test_membership_approval_trainer_denies()
{
  seed_fixture();
  id_t member_a = find_member_id(MEMBER_A);

  set_session_context(USER_ROLE_TRAINER, 1, "trA", BRANCH_A);
  assert(ensure_membership_approval_is_allowed(member_a) == false);
  clear_session_context();
}

void test_membership_approval_member_denies()
{
  seed_fixture();
  id_t member_a = find_member_id(MEMBER_A);

  set_session_context(USER_ROLE_MEMBER, member_a, MEMBER_A, BRANCH_A);
  assert(ensure_membership_approval_is_allowed(member_a) == false);
  clear_session_context();
}

void test_membership_approval_inactive_session_denies()
{
  seed_fixture();
  id_t member_a = find_member_id(MEMBER_A);

  assert(ensure_membership_approval_is_allowed(member_a) == false);
}

void test_membership_approval_unknown_member_id_denies()
{
  seed_fixture();
  id_t manager_a = find_staff_id(STAFF_A);

  set_session_context(USER_ROLE_BRANCH_MANAGER, manager_a, STAFF_A, BRANCH_A);
  assert(ensure_membership_approval_is_allowed(9999) == false);
  clear_session_context();
}

// ---- membership suspension ----

void test_membership_suspension_sysadmin_allows_any()
{
  seed_fixture();
  id_t member_b = find_member_id(MEMBER_B);

  set_session_context(USER_ROLE_SYSADMIN, 1, "admin", "");
  assert(ensure_membership_suspension_is_allowed(member_b) == true);
  clear_session_context();
}

void test_membership_suspension_manager_own_branch_allows()
{
  seed_fixture();
  id_t member_a = find_member_id(MEMBER_A);
  id_t manager_a = find_staff_id(STAFF_A);

  set_session_context(USER_ROLE_BRANCH_MANAGER, manager_a, STAFF_A, BRANCH_A);
  assert(ensure_membership_suspension_is_allowed(member_a) == true);
  clear_session_context();
}

void test_membership_suspension_manager_other_branch_denies()
{
  seed_fixture();
  id_t member_b = find_member_id(MEMBER_B);
  id_t manager_a = find_staff_id(STAFF_A);

  set_session_context(USER_ROLE_BRANCH_MANAGER, manager_a, STAFF_A, BRANCH_A);
  assert(ensure_membership_suspension_is_allowed(member_b) == false);
  clear_session_context();
}

void test_membership_suspension_trainer_denies()
{
  seed_fixture();
  id_t member_a = find_member_id(MEMBER_A);

  set_session_context(USER_ROLE_TRAINER, 1, "trA", BRANCH_A);
  assert(ensure_membership_suspension_is_allowed(member_a) == false);
  clear_session_context();
}

void test_membership_suspension_member_denies()
{
  seed_fixture();
  id_t member_a = find_member_id(MEMBER_A);

  set_session_context(USER_ROLE_MEMBER, member_a, MEMBER_A, BRANCH_A);
  assert(ensure_membership_suspension_is_allowed(member_a) == false);
  clear_session_context();
}

void test_membership_suspension_inactive_session_denies()
{
  seed_fixture();
  id_t member_a = find_member_id(MEMBER_A);

  assert(ensure_membership_suspension_is_allowed(member_a) == false);
}

void test_membership_suspension_unknown_member_id_denies()
{
  seed_fixture();
  id_t manager_a = find_staff_id(STAFF_A);

  set_session_context(USER_ROLE_BRANCH_MANAGER, manager_a, STAFF_A, BRANCH_A);
  assert(ensure_membership_suspension_is_allowed(9999) == false);
  clear_session_context();
}

// ---- membership unsuspension ----

void test_membership_unsuspension_sysadmin_allows_any()
{
  seed_fixture();
  id_t member_b = find_member_id(MEMBER_B);

  set_session_context(USER_ROLE_SYSADMIN, 1, "admin", "");
  assert(ensure_membership_unsuspension_is_allowed(member_b) == true);
  clear_session_context();
}

void test_membership_unsuspension_manager_own_branch_allows()
{
  seed_fixture();
  id_t member_a = find_member_id(MEMBER_A);
  id_t manager_a = find_staff_id(STAFF_A);

  set_session_context(USER_ROLE_BRANCH_MANAGER, manager_a, STAFF_A, BRANCH_A);
  assert(ensure_membership_unsuspension_is_allowed(member_a) == true);
  clear_session_context();
}

void test_membership_unsuspension_manager_other_branch_denies()
{
  seed_fixture();
  id_t member_b = find_member_id(MEMBER_B);
  id_t manager_a = find_staff_id(STAFF_A);

  set_session_context(USER_ROLE_BRANCH_MANAGER, manager_a, STAFF_A, BRANCH_A);
  assert(ensure_membership_unsuspension_is_allowed(member_b) == false);
  clear_session_context();
}

void test_membership_unsuspension_trainer_denies()
{
  seed_fixture();
  id_t member_a = find_member_id(MEMBER_A);

  set_session_context(USER_ROLE_TRAINER, 1, "trA", BRANCH_A);
  assert(ensure_membership_unsuspension_is_allowed(member_a) == false);
  clear_session_context();
}

void test_membership_unsuspension_member_denies()
{
  seed_fixture();
  id_t member_a = find_member_id(MEMBER_A);

  set_session_context(USER_ROLE_MEMBER, member_a, MEMBER_A, BRANCH_A);
  assert(ensure_membership_unsuspension_is_allowed(member_a) == false);
  clear_session_context();
}

void test_membership_unsuspension_inactive_session_denies()
{
  seed_fixture();
  id_t member_a = find_member_id(MEMBER_A);

  assert(ensure_membership_unsuspension_is_allowed(member_a) == false);
}

void test_membership_unsuspension_unknown_member_id_denies()
{
  seed_fixture();
  id_t manager_a = find_staff_id(STAFF_A);

  set_session_context(USER_ROLE_BRANCH_MANAGER, manager_a, STAFF_A, BRANCH_A);
  assert(ensure_membership_unsuspension_is_allowed(9999) == false);
  clear_session_context();
}

// ---- digital payment (member self) ----

void test_digital_payment_sysadmin_allows_any()
{
  seed_fixture();
  id_t member_a = find_member_id(MEMBER_A);

  set_session_context(USER_ROLE_SYSADMIN, 1, "admin", "");
  assert(ensure_digital_payment_is_allowed(member_a) == true);
  clear_session_context();
}

void test_digital_payment_member_self_allows()
{
  seed_fixture();
  id_t member_a = find_member_id(MEMBER_A);

  set_session_context(USER_ROLE_MEMBER, member_a, MEMBER_A, BRANCH_A);
  assert(ensure_digital_payment_is_allowed(member_a) == true);
  clear_session_context();
}

void test_digital_payment_member_other_denies()
{
  seed_fixture();
  id_t member_a = find_member_id(MEMBER_A);
  id_t member_b = find_member_id(MEMBER_B);

  set_session_context(USER_ROLE_MEMBER, member_b, MEMBER_B, BRANCH_B);
  assert(ensure_digital_payment_is_allowed(member_a) == false);
  clear_session_context();
}

void test_digital_payment_trainer_denies()
{
  seed_fixture();
  id_t member_a = find_member_id(MEMBER_A);

  set_session_context(USER_ROLE_TRAINER, 1, "trA", BRANCH_A);
  assert(ensure_digital_payment_is_allowed(member_a) == false);
  clear_session_context();
}

void test_digital_payment_inactive_session_denies()
{
  seed_fixture();
  id_t member_a = find_member_id(MEMBER_A);

  assert(ensure_digital_payment_is_allowed(member_a) == false);
}

// ---- cash payment (staff) ----

void test_cash_payment_sysadmin_allows_any()
{
  seed_fixture();
  id_t member_b = find_member_id(MEMBER_B);

  set_session_context(USER_ROLE_SYSADMIN, 1, "admin", "");
  assert(ensure_cash_payment_recording_is_allowed(member_b) == true);
  clear_session_context();
}

void test_cash_payment_manager_own_branch_allows()
{
  seed_fixture();
  id_t member_a = find_member_id(MEMBER_A);
  id_t manager_a = find_staff_id(STAFF_A);

  set_session_context(USER_ROLE_BRANCH_MANAGER, manager_a, STAFF_A, BRANCH_A);
  assert(ensure_cash_payment_recording_is_allowed(member_a) == true);
  clear_session_context();
}

void test_cash_payment_trainer_own_branch_allows()
{
  seed_fixture();
  id_t member_a = find_member_id(MEMBER_A);

  set_session_context(USER_ROLE_TRAINER, 1, "trA", BRANCH_A);
  assert(ensure_cash_payment_recording_is_allowed(member_a) == true);
  clear_session_context();
}

void test_cash_payment_manager_other_branch_denies()
{
  seed_fixture();
  id_t member_b = find_member_id(MEMBER_B);
  id_t manager_a = find_staff_id(STAFF_A);

  set_session_context(USER_ROLE_BRANCH_MANAGER, manager_a, STAFF_A, BRANCH_A);
  assert(ensure_cash_payment_recording_is_allowed(member_b) == false);
  clear_session_context();
}

void test_cash_payment_member_denies()
{
  seed_fixture();
  id_t member_a = find_member_id(MEMBER_A);

  set_session_context(USER_ROLE_MEMBER, member_a, MEMBER_A, BRANCH_A);
  assert(ensure_cash_payment_recording_is_allowed(member_a) == false);
  clear_session_context();
}

void test_cash_payment_inactive_session_denies()
{
  seed_fixture();
  id_t member_a = find_member_id(MEMBER_A);

  assert(ensure_cash_payment_recording_is_allowed(member_a) == false);
}

void test_cash_payment_unknown_member_id_denies()
{
  seed_fixture();
  id_t manager_a = find_staff_id(STAFF_A);

  set_session_context(USER_ROLE_BRANCH_MANAGER, manager_a, STAFF_A, BRANCH_A);
  assert(ensure_cash_payment_recording_is_allowed(9999) == false);
  clear_session_context();
}

// ---- payment view ----

void test_payment_view_sysadmin_allows_any()
{
  seed_fixture();
  id_t member_b = find_member_id(MEMBER_B);

  set_session_context(USER_ROLE_SYSADMIN, 1, "admin", "");
  assert(ensure_payment_view_allowed(member_b) == true);
  clear_session_context();
}

void test_payment_view_member_self_allows()
{
  seed_fixture();
  id_t member_a = find_member_id(MEMBER_A);

  set_session_context(USER_ROLE_MEMBER, member_a, MEMBER_A, BRANCH_A);
  assert(ensure_payment_view_allowed(member_a) == true);
  clear_session_context();
}

void test_payment_view_member_other_denies()
{
  seed_fixture();
  id_t member_a = find_member_id(MEMBER_A);
  id_t member_b = find_member_id(MEMBER_B);

  set_session_context(USER_ROLE_MEMBER, member_b, MEMBER_B, BRANCH_B);
  assert(ensure_payment_view_allowed(member_a) == false);
  clear_session_context();
}

void test_payment_view_manager_own_branch_allows()
{
  seed_fixture();
  id_t member_a = find_member_id(MEMBER_A);
  id_t manager_a = find_staff_id(STAFF_A);

  set_session_context(USER_ROLE_BRANCH_MANAGER, manager_a, STAFF_A, BRANCH_A);
  assert(ensure_payment_view_allowed(member_a) == true);
  clear_session_context();
}

void test_payment_view_manager_other_branch_denies()
{
  seed_fixture();
  id_t member_b = find_member_id(MEMBER_B);
  id_t manager_a = find_staff_id(STAFF_A);

  set_session_context(USER_ROLE_BRANCH_MANAGER, manager_a, STAFF_A, BRANCH_A);
  assert(ensure_payment_view_allowed(member_b) == false);
  clear_session_context();
}

void test_payment_view_inactive_session_denies()
{
  seed_fixture();
  id_t member_a = find_member_id(MEMBER_A);

  assert(ensure_payment_view_allowed(member_a) == false);
}

void test_payment_view_unknown_member_id_denies()
{
  seed_fixture();
  id_t manager_a = find_staff_id(STAFF_A);

  set_session_context(USER_ROLE_BRANCH_MANAGER, manager_a, STAFF_A, BRANCH_A);
  assert(ensure_payment_view_allowed(9999) == false);
  clear_session_context();
}

// ---- member profile view ----

void test_member_profile_view_sysadmin_allows_any()
{
  seed_fixture();
  id_t member_b = find_member_id(MEMBER_B);

  set_session_context(USER_ROLE_SYSADMIN, 1, "admin", "");
  assert(ensure_member_profile_view_allowed(member_b) == true);
  clear_session_context();
}

void test_member_profile_view_member_self_allows()
{
  seed_fixture();
  id_t member_a = find_member_id(MEMBER_A);

  set_session_context(USER_ROLE_MEMBER, member_a, MEMBER_A, BRANCH_A);
  assert(ensure_member_profile_view_allowed(member_a) == true);
  clear_session_context();
}

void test_member_profile_view_member_other_denies()
{
  seed_fixture();
  id_t member_a = find_member_id(MEMBER_A);
  id_t member_b = find_member_id(MEMBER_B);

  set_session_context(USER_ROLE_MEMBER, member_b, MEMBER_B, BRANCH_B);
  assert(ensure_member_profile_view_allowed(member_a) == false);
  clear_session_context();
}

void test_member_profile_view_manager_own_branch_allows()
{
  seed_fixture();
  id_t member_a = find_member_id(MEMBER_A);
  id_t manager_a = find_staff_id(STAFF_A);

  set_session_context(USER_ROLE_BRANCH_MANAGER, manager_a, STAFF_A, BRANCH_A);
  assert(ensure_member_profile_view_allowed(member_a) == true);
  clear_session_context();
}

void test_member_profile_view_manager_other_branch_denies()
{
  seed_fixture();
  id_t member_b = find_member_id(MEMBER_B);
  id_t manager_a = find_staff_id(STAFF_A);

  set_session_context(USER_ROLE_BRANCH_MANAGER, manager_a, STAFF_A, BRANCH_A);
  assert(ensure_member_profile_view_allowed(member_b) == false);
  clear_session_context();
}

void test_member_profile_view_inactive_session_denies()
{
  seed_fixture();
  id_t member_a = find_member_id(MEMBER_A);

  assert(ensure_member_profile_view_allowed(member_a) == false);
}

void test_member_profile_view_unknown_member_id_denies()
{
  seed_fixture();
  id_t manager_a = find_staff_id(STAFF_A);

  set_session_context(USER_ROLE_BRANCH_MANAGER, manager_a, STAFF_A, BRANCH_A);
  assert(ensure_member_profile_view_allowed(9999) == false);
  clear_session_context();
}

// ---- lost and found resolution ----

void test_lost_found_resolution_sysadmin_allows_any()
{
  seed_fixture();
  lost_and_found_record_t item = {0};
  item.id = 100;
  strcpy(item.gym_branch, BRANCH_B);

  set_session_context(USER_ROLE_SYSADMIN, 1, "admin", "");
  assert(ensure_lost_found_resolution_is_allowed(item) == true);
  clear_session_context();
}

void test_lost_found_resolution_manager_own_branch_allows()
{
  seed_fixture();
  lost_and_found_record_t item = {0};
  item.id = 101;
  strcpy(item.gym_branch, BRANCH_A);

  id_t manager_a = find_staff_id(STAFF_A);
  set_session_context(USER_ROLE_BRANCH_MANAGER, manager_a, STAFF_A, BRANCH_A);
  assert(ensure_lost_found_resolution_is_allowed(item) == true);
  clear_session_context();
}

void test_lost_found_resolution_manager_other_branch_denies()
{
  seed_fixture();
  lost_and_found_record_t item = {0};
  item.id = 102;
  strcpy(item.gym_branch, BRANCH_B);

  id_t manager_a = find_staff_id(STAFF_A);
  set_session_context(USER_ROLE_BRANCH_MANAGER, manager_a, STAFF_A, BRANCH_A);
  assert(ensure_lost_found_resolution_is_allowed(item) == false);
  clear_session_context();
}

void test_lost_found_resolution_trainer_denies()
{
  seed_fixture();
  lost_and_found_record_t item = {0};
  item.id = 103;
  strcpy(item.gym_branch, BRANCH_A);

  set_session_context(USER_ROLE_TRAINER, 1, "trA", BRANCH_A);
  assert(ensure_lost_found_resolution_is_allowed(item) == false);
  clear_session_context();
}

void test_lost_found_resolution_inactive_session_denies()
{
  seed_fixture();
  lost_and_found_record_t item = {0};
  item.id = 104;
  strcpy(item.gym_branch, BRANCH_A);

  assert(ensure_lost_found_resolution_is_allowed(item) == false);
}

// ---- branch deletion ----

void test_branch_deletion_sysadmin_allows()
{
  seed_fixture();

  set_session_context(USER_ROLE_SYSADMIN, 1, "admin", "");
  assert(ensure_branch_deletion_is_allowed(BRANCH_A) == true);
  clear_session_context();
}

void test_branch_deletion_manager_denies()
{
  seed_fixture();
  id_t manager_a = find_staff_id(STAFF_A);

  set_session_context(USER_ROLE_BRANCH_MANAGER, manager_a, STAFF_A, BRANCH_A);
  assert(ensure_branch_deletion_is_allowed(BRANCH_A) == false);
  clear_session_context();
}

void test_branch_deletion_trainer_denies()
{
  seed_fixture();

  set_session_context(USER_ROLE_TRAINER, 1, "trA", BRANCH_A);
  assert(ensure_branch_deletion_is_allowed(BRANCH_A) == false);
  clear_session_context();
}

void test_branch_deletion_member_denies()
{
  seed_fixture();
  id_t member_a = find_member_id(MEMBER_A);

  set_session_context(USER_ROLE_MEMBER, member_a, MEMBER_A, BRANCH_A);
  assert(ensure_branch_deletion_is_allowed(BRANCH_A) == false);
  clear_session_context();
}

void test_branch_deletion_inactive_session_denies()
{
  seed_fixture();
  assert(ensure_branch_deletion_is_allowed(BRANCH_A) == false);
}

void test_branch_deletion_blank_branch_denies()
{
  seed_fixture();
  set_session_context(USER_ROLE_SYSADMIN, 1, "admin", "");
  assert(ensure_branch_deletion_is_allowed("") == false);
  assert(ensure_branch_deletion_is_allowed("   ") == false);
  clear_session_context();
}

void test_branch_deletion_null_branch_denies()
{
  seed_fixture();
  set_session_context(USER_ROLE_SYSADMIN, 1, "admin", "");
  assert(ensure_branch_deletion_is_allowed(NULL) == false);
  clear_session_context();
}

// ---- member deletion ----

void test_member_deletion_sysadmin_allows_any()
{
  seed_fixture();
  id_t member_b = find_member_id(MEMBER_B);

  set_session_context(USER_ROLE_SYSADMIN, 1, "admin", "");
  assert(ensure_member_deletion_is_allowed(member_b) == true);
  clear_session_context();
}

void test_member_deletion_manager_own_branch_zero_dues_allows()
{
  seed_fixture();
  id_t member_a = find_member_id(MEMBER_A);
  id_t manager_a = find_staff_id(STAFF_A);

  set_session_context(USER_ROLE_BRANCH_MANAGER, manager_a, STAFF_A, BRANCH_A);
  assert(ensure_member_deletion_is_allowed(member_a) == true);
  clear_session_context();
}

void test_member_deletion_manager_own_branch_with_dues_denies()
{
  seed_fixture();
  id_t manager_a = find_staff_id(STAFF_A);

  // Inject a member under branch A with a non-zero due_amount.
  write_raw_member_with_dues(555, "indebtedA", 500, MEMBERSHIP_ACTIVE);
  assert(load_gym_members() >= 3);
  gym_member_t m;
  assert(get_gym_member_by_id(555, &m) == true);
  assert(m.due_amount == 500);

  set_session_context(USER_ROLE_BRANCH_MANAGER, manager_a, STAFF_A, BRANCH_A);
  assert(ensure_member_deletion_is_allowed(555) == false);
  clear_session_context();
}

void test_member_deletion_manager_other_branch_denies()
{
  seed_fixture();
  id_t member_b = find_member_id(MEMBER_B);
  id_t manager_a = find_staff_id(STAFF_A);

  set_session_context(USER_ROLE_BRANCH_MANAGER, manager_a, STAFF_A, BRANCH_A);
  assert(ensure_member_deletion_is_allowed(member_b) == false);
  clear_session_context();
}

void test_member_deletion_trainer_denies()
{
  seed_fixture();
  id_t member_a = find_member_id(MEMBER_A);

  set_session_context(USER_ROLE_TRAINER, 1, "trA", BRANCH_A);
  assert(ensure_member_deletion_is_allowed(member_a) == false);
  clear_session_context();
}

void test_member_deletion_inactive_session_denies()
{
  seed_fixture();
  id_t member_a = find_member_id(MEMBER_A);

  assert(ensure_member_deletion_is_allowed(member_a) == false);
}

void test_member_deletion_unknown_member_id_denies()
{
  seed_fixture();
  id_t manager_a = find_staff_id(STAFF_A);

  set_session_context(USER_ROLE_BRANCH_MANAGER, manager_a, STAFF_A, BRANCH_A);
  assert(ensure_member_deletion_is_allowed(9999) == false);
  clear_session_context();
}

// ---- staff deletion ----

void test_staff_deletion_sysadmin_allows_any()
{
  seed_fixture();
  id_t manager_b = find_staff_id(STAFF_B);

  set_session_context(USER_ROLE_SYSADMIN, 1, "admin", "");
  assert(ensure_staff_deletion_is_allowed(manager_b) == true);
  clear_session_context();
}

void test_staff_deletion_manager_own_branch_trainer_allows()
{
  seed_fixture();
  // Add a trainer under branch A so the deletion has a valid target.
  id_t trainer = create_branch_staff("Trainer A", "trA@test.com", "01844444444", BRANCH_A, "trainerA", "h", TRAINER);
  assert(trainer != 0);
  id_t manager_a = find_staff_id(STAFF_A);

  set_session_context(USER_ROLE_BRANCH_MANAGER, manager_a, STAFF_A, BRANCH_A);
  assert(ensure_staff_deletion_is_allowed(trainer) == true);
  clear_session_context();
}

void test_staff_deletion_manager_own_branch_manager_denies()
{
  seed_fixture();
  id_t manager_b = find_staff_id(STAFF_B);
  id_t manager_a = find_staff_id(STAFF_A);

  set_session_context(USER_ROLE_BRANCH_MANAGER, manager_a, STAFF_A, BRANCH_A);
  // Cross-branch first; should fail before the manager-role check is even reached.
  assert(ensure_staff_deletion_is_allowed(manager_b) == false);

  // Set up a second manager under branch A so the same-branch manager-role check fires.
  id_t second_mgr =
    create_branch_staff("Second Mgr A", "mgr2A@test.com", "01855555555", BRANCH_A, "secondA", "h", BRANCH_MANAGER);
  assert(second_mgr != 0);
  assert(ensure_staff_deletion_is_allowed(second_mgr) == false);
  clear_session_context();
}

void test_staff_deletion_manager_other_branch_denies()
{
  seed_fixture();
  id_t manager_b = find_staff_id(STAFF_B);
  id_t manager_a = find_staff_id(STAFF_A);

  set_session_context(USER_ROLE_BRANCH_MANAGER, manager_a, STAFF_A, BRANCH_A);
  assert(ensure_staff_deletion_is_allowed(manager_b) == false);
  clear_session_context();
}

void test_staff_deletion_trainer_denies()
{
  seed_fixture();
  id_t manager_a = find_staff_id(STAFF_A);

  set_session_context(USER_ROLE_TRAINER, 1, "trA", BRANCH_A);
  assert(ensure_staff_deletion_is_allowed(manager_a) == false);
  clear_session_context();
}

void test_staff_deletion_inactive_session_denies()
{
  seed_fixture();
  id_t manager_a = find_staff_id(STAFF_A);

  assert(ensure_staff_deletion_is_allowed(manager_a) == false);
}

void test_staff_deletion_unknown_staff_id_denies()
{
  seed_fixture();
  id_t manager_a = find_staff_id(STAFF_A);

  set_session_context(USER_ROLE_BRANCH_MANAGER, manager_a, STAFF_A, BRANCH_A);
  assert(ensure_staff_deletion_is_allowed(9999) == false);
  clear_session_context();
}

// ---- branch creation ----

void test_branch_creation_sysadmin_allows()
{
  seed_fixture();
  // Reset to a low-fill state by ensuring we are below the cap.
  set_session_context(USER_ROLE_SYSADMIN, 1, "admin", "");
  assert(ensure_branch_creation_is_allowed() == true);
  clear_session_context();
}

void test_branch_creation_manager_denies()
{
  seed_fixture();
  id_t manager_a = find_staff_id(STAFF_A);

  set_session_context(USER_ROLE_BRANCH_MANAGER, manager_a, STAFF_A, BRANCH_A);
  assert(ensure_branch_creation_is_allowed() == false);
  clear_session_context();
}

void test_branch_creation_trainer_denies()
{
  seed_fixture();
  set_session_context(USER_ROLE_TRAINER, 1, "trA", BRANCH_A);
  assert(ensure_branch_creation_is_allowed() == false);
  clear_session_context();
}

void test_branch_creation_member_denies()
{
  seed_fixture();
  id_t member_a = find_member_id(MEMBER_A);
  set_session_context(USER_ROLE_MEMBER, member_a, MEMBER_A, BRANCH_A);
  assert(ensure_branch_creation_is_allowed() == false);
  clear_session_context();
}

void test_branch_creation_inactive_session_denies()
{
  seed_fixture();
  assert(ensure_branch_creation_is_allowed() == false);
}

// ---- branch rename ----

void test_branch_rename_sysadmin_allows()
{
  seed_fixture();
  set_session_context(USER_ROLE_SYSADMIN, 1, "admin", "");
  assert(ensure_branch_rename_is_allowed() == true);
  clear_session_context();
}

void test_branch_rename_manager_denies()
{
  seed_fixture();
  id_t manager_a = find_staff_id(STAFF_A);
  set_session_context(USER_ROLE_BRANCH_MANAGER, manager_a, STAFF_A, BRANCH_A);
  assert(ensure_branch_rename_is_allowed() == false);
  clear_session_context();
}

void test_branch_rename_trainer_denies()
{
  seed_fixture();
  set_session_context(USER_ROLE_TRAINER, 1, "trA", BRANCH_A);
  assert(ensure_branch_rename_is_allowed() == false);
  clear_session_context();
}

void test_branch_rename_member_denies()
{
  seed_fixture();
  id_t member_a = find_member_id(MEMBER_A);
  set_session_context(USER_ROLE_MEMBER, member_a, MEMBER_A, BRANCH_A);
  assert(ensure_branch_rename_is_allowed() == false);
  clear_session_context();
}

void test_branch_rename_inactive_session_denies()
{
  seed_fixture();
  assert(ensure_branch_rename_is_allowed() == false);
}

// ---- staff creation ----

void test_staff_creation_sysadmin_trainer_any_branch_allows()
{
  seed_fixture();
  set_session_context(USER_ROLE_SYSADMIN, 1, "admin", "");
  assert(ensure_staff_creation_is_allowed(BRANCH_A, TRAINER) == true);
  assert(ensure_staff_creation_is_allowed(BRANCH_B, TRAINER) == true);
  clear_session_context();
}

void test_staff_creation_sysadmin_manager_any_branch_allows()
{
  seed_fixture();
  set_session_context(USER_ROLE_SYSADMIN, 1, "admin", "");
  // Branch A already has a manager; that slot is full so a second is denied.
  assert(ensure_staff_creation_is_allowed(BRANCH_A, BRANCH_MANAGER) == false);
  // Branch B already has a manager; same.
  assert(ensure_staff_creation_is_allowed(BRANCH_B, BRANCH_MANAGER) == false);
  // Add a third empty branch to confirm the cap is the only blocker.
  add_branch("EmptyBranch");
  assert(ensure_staff_creation_is_allowed("EmptyBranch", BRANCH_MANAGER) == true);
  clear_session_context();
}

void test_staff_creation_manager_trainer_own_branch_allows()
{
  seed_fixture();
  id_t manager_a = find_staff_id(STAFF_A);
  set_session_context(USER_ROLE_BRANCH_MANAGER, manager_a, STAFF_A, BRANCH_A);
  assert(ensure_staff_creation_is_allowed(BRANCH_A, TRAINER) == true);
  clear_session_context();
}

void test_staff_creation_manager_manager_own_branch_denies()
{
  seed_fixture();
  id_t manager_a = find_staff_id(STAFF_A);
  set_session_context(USER_ROLE_BRANCH_MANAGER, manager_a, STAFF_A, BRANCH_A);
  assert(ensure_staff_creation_is_allowed(BRANCH_A, BRANCH_MANAGER) == false);
  clear_session_context();
}

void test_staff_creation_manager_trainer_other_branch_denies()
{
  seed_fixture();
  id_t manager_a = find_staff_id(STAFF_A);
  set_session_context(USER_ROLE_BRANCH_MANAGER, manager_a, STAFF_A, BRANCH_A);
  assert(ensure_staff_creation_is_allowed(BRANCH_B, TRAINER) == false);
  clear_session_context();
}

void test_staff_creation_trainer_denies()
{
  seed_fixture();
  set_session_context(USER_ROLE_TRAINER, 1, "trA", BRANCH_A);
  assert(ensure_staff_creation_is_allowed(BRANCH_A, TRAINER) == false);
  clear_session_context();
}

void test_staff_creation_member_denies()
{
  seed_fixture();
  id_t member_a = find_member_id(MEMBER_A);
  set_session_context(USER_ROLE_MEMBER, member_a, MEMBER_A, BRANCH_A);
  assert(ensure_staff_creation_is_allowed(BRANCH_A, TRAINER) == false);
  clear_session_context();
}

void test_staff_creation_inactive_session_denies()
{
  seed_fixture();
  assert(ensure_staff_creation_is_allowed(BRANCH_A, TRAINER) == false);
}

void test_staff_creation_blank_branch_denies()
{
  seed_fixture();
  set_session_context(USER_ROLE_SYSADMIN, 1, "admin", "");
  assert(ensure_staff_creation_is_allowed("", TRAINER) == false);
  assert(ensure_staff_creation_is_allowed("   ", TRAINER) == false);
  clear_session_context();
}

void test_staff_creation_unknown_branch_denies()
{
  seed_fixture();
  set_session_context(USER_ROLE_SYSADMIN, 1, "admin", "");
  assert(ensure_staff_creation_is_allowed("NotARealBranch", TRAINER) == false);
  clear_session_context();
}

void test_staff_creation_invalid_role_denies()
{
  seed_fixture();
  set_session_context(USER_ROLE_SYSADMIN, 1, "admin", "");
  assert(ensure_staff_creation_is_allowed(BRANCH_A, 99) == false);
  clear_session_context();
}

// ---- gym member creation ----

void test_gym_member_creation_blank_branch_denies()
{
  seed_fixture();
  set_session_context(USER_ROLE_SYSADMIN, 1, "admin", "");
  assert(ensure_gym_member_creation_is_allowed("") == false);
  assert(ensure_gym_member_creation_is_allowed(NULL) == false);
  clear_session_context();
}

void test_gym_member_creation_unknown_branch_denies()
{
  seed_fixture();
  set_session_context(USER_ROLE_SYSADMIN, 1, "admin", "");
  assert(ensure_gym_member_creation_is_allowed("NotARealBranch") == false);
  clear_session_context();
}

void test_gym_member_creation_valid_branch_allows()
{
  seed_fixture();
  set_session_context(USER_ROLE_SYSADMIN, 1, "admin", "");
  assert(ensure_gym_member_creation_is_allowed(BRANCH_A) == true);
  clear_session_context();
}

// ---- branch name validity ----

void test_branch_name_is_valid_blank_denies()
{
  seed_fixture();
  assert(ensure_branch_name_is_valid("") == false);
  assert(ensure_branch_name_is_valid("   ") == false);
}

void test_branch_name_is_valid_null_denies()
{
  seed_fixture();
  assert(ensure_branch_name_is_valid(NULL) == false);
}

void test_branch_name_is_valid_unknown_denies()
{
  seed_fixture();
  assert(ensure_branch_name_is_valid("NotARealBranch") == false);
}

void test_branch_name_is_valid_known_allows()
{
  seed_fixture();
  assert(ensure_branch_name_is_valid(BRANCH_A) == true);
  assert(ensure_branch_name_is_valid(BRANCH_B) == true);
}

// ---- branch listing ----

void test_branch_listing_sysadmin_allows()
{
  seed_fixture();
  set_session_context(USER_ROLE_SYSADMIN, 1, "admin", "");
  assert(ensure_branch_listing_is_allowed() == true);
  clear_session_context();
}

void test_branch_listing_manager_allows()
{
  seed_fixture();
  id_t manager_a = find_staff_id(STAFF_A);
  set_session_context(USER_ROLE_BRANCH_MANAGER, manager_a, STAFF_A, BRANCH_A);
  assert(ensure_branch_listing_is_allowed() == true);
  clear_session_context();
}

void test_branch_listing_member_allows()
{
  seed_fixture();
  id_t member_a = find_member_id(MEMBER_A);
  set_session_context(USER_ROLE_MEMBER, member_a, MEMBER_A, BRANCH_A);
  assert(ensure_branch_listing_is_allowed() == true);
  clear_session_context();
}

void test_branch_listing_inactive_session_denies()
{
  seed_fixture();
  assert(ensure_branch_listing_is_allowed() == false);
}

// ---- member listing ----

void test_member_listing_sysadmin_any_branch_allows()
{
  seed_fixture();
  set_session_context(USER_ROLE_SYSADMIN, 1, "admin", "");
  assert(ensure_member_listing_is_allowed(BRANCH_A) == true);
  assert(ensure_member_listing_is_allowed(BRANCH_B) == true);
  clear_session_context();
}

void test_member_listing_sysadmin_empty_branch_allows()
{
  seed_fixture();
  set_session_context(USER_ROLE_SYSADMIN, 1, "admin", "");
  assert(ensure_member_listing_is_allowed("") == true);
  clear_session_context();
}

void test_member_listing_manager_own_branch_allows()
{
  seed_fixture();
  id_t manager_a = find_staff_id(STAFF_A);
  set_session_context(USER_ROLE_BRANCH_MANAGER, manager_a, STAFF_A, BRANCH_A);
  assert(ensure_member_listing_is_allowed(BRANCH_A) == true);
  clear_session_context();
}

void test_member_listing_manager_other_branch_denies()
{
  seed_fixture();
  id_t manager_a = find_staff_id(STAFF_A);
  set_session_context(USER_ROLE_BRANCH_MANAGER, manager_a, STAFF_A, BRANCH_A);
  assert(ensure_member_listing_is_allowed(BRANCH_B) == false);
  clear_session_context();
}

void test_member_listing_trainer_own_branch_allows()
{
  seed_fixture();
  set_session_context(USER_ROLE_TRAINER, 1, "trA", BRANCH_A);
  assert(ensure_member_listing_is_allowed(BRANCH_A) == true);
  clear_session_context();
}

void test_member_listing_trainer_other_branch_denies()
{
  seed_fixture();
  set_session_context(USER_ROLE_TRAINER, 1, "trA", BRANCH_A);
  assert(ensure_member_listing_is_allowed(BRANCH_B) == false);
  clear_session_context();
}

void test_member_listing_inactive_session_denies()
{
  seed_fixture();
  assert(ensure_member_listing_is_allowed(BRANCH_A) == false);
}

void test_member_listing_blank_branch_non_sysadmin_denies()
{
  seed_fixture();
  id_t manager_a = find_staff_id(STAFF_A);
  set_session_context(USER_ROLE_BRANCH_MANAGER, manager_a, STAFF_A, BRANCH_A);
  assert(ensure_member_listing_is_allowed("") == false);
  clear_session_context();
}

// ---- lost and found view ----

void test_lost_found_view_sysadmin_any_branch_allows()
{
  seed_fixture();
  set_session_context(USER_ROLE_SYSADMIN, 1, "admin", "");
  assert(ensure_lost_found_view_is_allowed(BRANCH_A) == true);
  assert(ensure_lost_found_view_is_allowed(BRANCH_B) == true);
  clear_session_context();
}

void test_lost_found_view_manager_own_branch_allows()
{
  seed_fixture();
  id_t manager_a = find_staff_id(STAFF_A);
  set_session_context(USER_ROLE_BRANCH_MANAGER, manager_a, STAFF_A, BRANCH_A);
  assert(ensure_lost_found_view_is_allowed(BRANCH_A) == true);
  clear_session_context();
}

void test_lost_found_view_manager_other_branch_denies()
{
  seed_fixture();
  id_t manager_a = find_staff_id(STAFF_A);
  set_session_context(USER_ROLE_BRANCH_MANAGER, manager_a, STAFF_A, BRANCH_A);
  assert(ensure_lost_found_view_is_allowed(BRANCH_B) == false);
  clear_session_context();
}

void test_lost_found_view_trainer_own_branch_allows()
{
  seed_fixture();
  set_session_context(USER_ROLE_TRAINER, 1, "trA", BRANCH_A);
  assert(ensure_lost_found_view_is_allowed(BRANCH_A) == true);
  clear_session_context();
}

void test_lost_found_view_trainer_other_branch_denies()
{
  seed_fixture();
  set_session_context(USER_ROLE_TRAINER, 1, "trA", BRANCH_A);
  assert(ensure_lost_found_view_is_allowed(BRANCH_B) == false);
  clear_session_context();
}

void test_lost_found_view_inactive_session_denies()
{
  seed_fixture();
  assert(ensure_lost_found_view_is_allowed(BRANCH_A) == false);
}

void test_lost_found_view_blank_branch_non_sysadmin_denies()
{
  seed_fixture();
  id_t manager_a = find_staff_id(STAFF_A);
  set_session_context(USER_ROLE_BRANCH_MANAGER, manager_a, STAFF_A, BRANCH_A);
  assert(ensure_lost_found_view_is_allowed("") == false);
  clear_session_context();
}

// ---- lost and found report ----

void test_lost_found_report_sysadmin_any_branch_allows()
{
  seed_fixture();
  set_session_context(USER_ROLE_SYSADMIN, 1, "admin", "");
  assert(ensure_lost_found_report_is_allowed(BRANCH_A) == true);
  assert(ensure_lost_found_report_is_allowed(BRANCH_B) == true);
  clear_session_context();
}

void test_lost_found_report_manager_own_branch_allows()
{
  seed_fixture();
  id_t manager_a = find_staff_id(STAFF_A);
  set_session_context(USER_ROLE_BRANCH_MANAGER, manager_a, STAFF_A, BRANCH_A);
  assert(ensure_lost_found_report_is_allowed(BRANCH_A) == true);
  clear_session_context();
}

void test_lost_found_report_manager_other_branch_denies()
{
  seed_fixture();
  id_t manager_a = find_staff_id(STAFF_A);
  set_session_context(USER_ROLE_BRANCH_MANAGER, manager_a, STAFF_A, BRANCH_A);
  assert(ensure_lost_found_report_is_allowed(BRANCH_B) == false);
  clear_session_context();
}

void test_lost_found_report_member_own_branch_allows()
{
  seed_fixture();
  id_t member_a = find_member_id(MEMBER_A);
  set_session_context(USER_ROLE_MEMBER, member_a, MEMBER_A, BRANCH_A);
  assert(ensure_lost_found_report_is_allowed(BRANCH_A) == true);
  clear_session_context();
}

void test_lost_found_report_member_other_branch_denies()
{
  seed_fixture();
  id_t member_b = find_member_id(MEMBER_B);
  set_session_context(USER_ROLE_MEMBER, member_b, MEMBER_B, BRANCH_B);
  assert(ensure_lost_found_report_is_allowed(BRANCH_A) == false);
  clear_session_context();
}

void test_lost_found_report_inactive_session_denies()
{
  seed_fixture();
  assert(ensure_lost_found_report_is_allowed(BRANCH_A) == false);
}

void test_lost_found_report_blank_branch_denies()
{
  seed_fixture();
  set_session_context(USER_ROLE_SYSADMIN, 1, "admin", "");
  assert(ensure_lost_found_report_is_allowed("") == false);
  assert(ensure_lost_found_report_is_allowed(NULL) == false);
  clear_session_context();
}

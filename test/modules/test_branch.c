#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "../../src/modules/branch.h"
#include "../../src/modules/user.h"
#include "../../src/settings.h"
#include "../../src/types.h"
#include "test_branch.h"

/**
 * Removes all user data files so branch policy tests start clean.
 */
static void cleanup_user_data_files()
{
  remove(SYSADMINS_FILE_PATH);
  remove(BRANCH_STAFF_FILE_PATH);
  remove(GYM_MEMBERS_FILE_PATH);
}

/**
 * Removes the test branches file from the test_data directory.
 * Called once at startup from test_main, not between individual tests.
 */
void cleanup_branches_file()
{
  remove(GYM_BRANCHES_FILE_PATH);
}

/**
 * Verifies that add_branch adds a branch and branch_exists finds it.
 */
void test_add_branch_and_exists()
{
  cleanup_branches_file();
  load_branches();

  assert(add_branch("Dhanmondi") == true);
  assert(branch_exists("Dhanmondi") == true);
}

/**
 * Verifies that add_branch rejects a duplicate branch name.
 */
void test_add_branch_rejects_duplicate()
{
  assert(add_branch("Dhanmondi") == false);
}

/**
 * Verifies that add_branch rejects an empty string.
 */
void test_add_branch_rejects_empty()
{
  assert(add_branch("") == false);
}

/**
 * Verifies that add_branch rejects a NULL input.
 */
void test_add_branch_rejects_null()
{
  assert(add_branch(NULL) == false);
}

/**
 * Verifies that branch_exists returns false for a non-existent branch.
 */
void test_branch_exists_returns_false_for_missing()
{
  assert(branch_exists("NonExistent") == false);
}

/**
 * Verifies that branch_exists returns false for NULL input.
 */
void test_branch_exists_returns_false_for_null()
{
  assert(branch_exists(NULL) == false);
}

/**
 * Verifies that load_branches loads all added branches.
 */
void test_load_branches()
{
  cleanup_branches_file();
  load_branches();

  add_branch("Banani");
  add_branch("Mirpur");
  add_branch("Bashundhara");

  load_branches();

  assert(get_branch_count() == 3);
  assert(strcmp(get_branch_name(0), "Banani") == 0);
  assert(strcmp(get_branch_name(1), "Mirpur") == 0);
  assert(strcmp(get_branch_name(2), "Bashundhara") == 0);
}

/**
 * Verifies that load_branches returns 0 when no branches exist.
 */
void test_load_branches_returns_zero_when_empty()
{
  cleanup_branches_file();
  load_branches();

  assert(get_branch_count() == 0);
}

/**
 * Verifies that add_branch respects the maximum branch count.
 */
void test_add_branch_respects_max_count()
{
  cleanup_branches_file();
  load_branches();

  char name[BRANCH_NAME_BUFFER_SIZE];
  for (int index = 0; index < BRANCH_COUNT_MAX; index++)
  {
    snprintf(name, BRANCH_NAME_BUFFER_SIZE, "Branch%d", index);
    assert(add_branch(name) == true);
  }

  assert(add_branch("OverflowBranch") == false);
}

/**
 * Verifies that branch names are stored and compared exactly.
 */
void test_branch_names_are_case_sensitive()
{
  cleanup_branches_file();
  load_branches();

  add_branch("Dhanmondi");
  assert(branch_exists("Dhanmondi") == true);
  assert(branch_exists("dhanmondi") == false);
  assert(branch_exists("DHANMONDI") == false);
}

/**
 * Verifies that get_branch_count returns the correct count.
 */
void test_get_branch_count()
{
  cleanup_branches_file();
  load_branches();

  assert(get_branch_count() == 0);

  add_branch("One");
  assert(get_branch_count() == 1);

  add_branch("Two");
  assert(get_branch_count() == 2);
}

/**
 * Verifies that get_branch_name returns NULL for invalid indices.
 */
void test_get_branch_name()
{
  cleanup_branches_file();
  load_branches();

  add_branch("Alpha");
  add_branch("Beta");

  assert(get_branch_name(0) != NULL);
  assert(strcmp(get_branch_name(0), "Alpha") == 0);
  assert(get_branch_name(1) != NULL);
  assert(strcmp(get_branch_name(1), "Beta") == 0);
  assert(get_branch_name(2) == NULL);
  assert(get_branch_name(-1) == NULL);
}

/**
 * Verifies the no-users policy passes for an empty branch and fails once
 * any staff is assigned.
 */
void test_ensure_branch_has_no_users()
{
  cleanup_branches_file();
  cleanup_user_data_files();
  load_branches();
  load_sysadmins();
  load_branch_staff();
  load_gym_members();

  add_branch("QuietBranch");
  assert(ensure_branch_has_no_users("QuietBranch") == true);

  id_t staff_id =
    create_branch_staff("Quiet Staff", "quiet@t.com", "0171111111", "QuietBranch", "quietstaff", "hash", TRAINER);
  assert(staff_id != 0);
  assert(ensure_branch_has_no_users("QuietBranch") == false);

  assert(delete_branch_staff(staff_id) == true);
  assert(ensure_branch_has_no_users("QuietBranch") == true);
}

/**
 * Verifies that delete_branch removes the branch from memory and disk and
 * keeps the remaining branches in order.
 */
void test_delete_branch_removes_and_persists()
{
  cleanup_branches_file();
  load_branches();

  add_branch("Uttara");
  add_branch("Gulshan");
  add_branch("Mohakhali");

  assert(delete_branch("Gulshan") == true);
  assert(branch_exists("Gulshan") == false);
  assert(get_branch_count() == 2);
  assert(strcmp(get_branch_name(0), "Uttara") == 0);
  assert(strcmp(get_branch_name(1), "Mohakhali") == 0);

  load_branches();
  assert(get_branch_count() == 2);
  assert(branch_exists("Gulshan") == false);
  assert(branch_exists("Uttara") == true);
  assert(branch_exists("Mohakhali") == true);
}

/**
 * Verifies that delete_branch rejects missing, NULL, and empty names.
 */
void test_delete_branch_rejects_missing_null_and_empty()
{
  cleanup_branches_file();
  load_branches();

  add_branch("ExistingBranch");

  assert(delete_branch("NoSuchBranch") == false);
  assert(delete_branch(NULL) == false);
  assert(delete_branch("") == false);
  assert(branch_exists("ExistingBranch") == true);
}

/**
 * Verifies that delete_branch rejects a branch that still has assigned
 * users and accepts it once they are removed.
 */
void test_delete_branch_rejects_branch_with_users()
{
  cleanup_branches_file();
  cleanup_user_data_files();
  load_branches();
  load_sysadmins();
  load_branch_staff();
  load_gym_members();

  add_branch("BusyBranch");

  subscription_plan_t plan;
  plan.payable_amount = 1000;
  plan.interval_days = 30;

  id_t member_id = create_gym_member(
    "Busy Member", "busy@t.com", "0172222222", "BusyBranch", "busymember", "hash", plan, MEMBERSHIP_ACTIVE
  );
  assert(member_id != 0);

  assert(delete_branch("BusyBranch") == false);
  assert(branch_exists("BusyBranch") == true);

  assert(delete_gym_member(member_id) == true);
  assert(delete_branch("BusyBranch") == true);
}

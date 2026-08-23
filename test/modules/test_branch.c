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
 * Verifies that add_branch adds a branch and find_branch locates it.
 */
void test_add_branch_and_exists()
{
  cleanup_branches_file();
  load_branches();

  assert(add_branch("Dhanmondi") == true);
  assert(find_branch("Dhanmondi") != -1);
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
 * Verifies that find_branch returns -1 for a non-existent branch.
 */
void test_find_branch_returns_negative_for_missing()
{
  assert(find_branch("NonExistent") == -1);
}

/**
 * Verifies that find_branch returns -1 for NULL input.
 */
void test_find_branch_returns_negative_for_null()
{
  assert(find_branch(NULL) == -1);
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
  assert(find_branch("Dhanmondi") != -1);
  assert(find_branch("dhanmondi") == -1);
  assert(find_branch("DHANMONDI") == -1);
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
  assert(find_branch("Gulshan") == -1);
  assert(get_branch_count() == 2);
  assert(strcmp(get_branch_name(0), "Uttara") == 0);
  assert(strcmp(get_branch_name(1), "Mohakhali") == 0);

  load_branches();
  assert(get_branch_count() == 2);
  assert(find_branch("Gulshan") == -1);
  assert(find_branch("Uttara") != -1);
  assert(find_branch("Mohakhali") != -1);
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
  assert(find_branch("ExistingBranch") != -1);
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
  assert(find_branch("BusyBranch") != -1);

  assert(delete_gym_member(member_id) == true);
  assert(delete_branch("BusyBranch") == true);
}

/**
 * Verifies that update_branch_name renames the branch in memory and on disk
 * while keeping the other branches in order.
 */
void test_update_branch_name_renames_and_persists()
{
  cleanup_branches_file();
  load_branches();

  add_branch("Uttara");
  add_branch("Gulshan");
  add_branch("Mohakhali");

  assert(update_branch_name("Gulshan", "Gulshan2") == true);
  assert(find_branch("Gulshan") == -1);
  assert(find_branch("Gulshan2") != -1);
  // The rename must happen in place at the original index.
  assert(find_branch("Uttara") == 0);
  assert(find_branch("Gulshan2") == 1);
  assert(find_branch("Mohakhali") == 2);
  assert(get_branch_count() == 3);
  assert(strcmp(get_branch_name(0), "Uttara") == 0);
  assert(strcmp(get_branch_name(1), "Gulshan2") == 0);
  assert(strcmp(get_branch_name(2), "Mohakhali") == 0);

  load_branches();
  assert(get_branch_count() == 3);
  assert(find_branch("Gulshan") == -1);
  assert(find_branch("Gulshan2") != -1);
}

/**
 * Verifies that renaming a branch cascades into staff and member records so
 * their gym_branch follows the new name, in memory and on disk, while
 * records of untouched branches stay as they were.
 */
void test_update_branch_name_cascades_to_user_records()
{
  cleanup_branches_file();
  cleanup_user_data_files();
  load_branches();
  load_sysadmins();
  load_branch_staff();
  load_gym_members();

  add_branch("RenameOld");
  add_branch("RenameOther");

  subscription_plan_t plan;
  plan.payable_amount = 1000;
  plan.interval_days = 30;

  id_t manager_id = create_branch_staff(
    "Cascade Manager", "cm@t.com", "0171111111", "RenameOld", "cascademanager", "hash", BRANCH_MANAGER
  );
  id_t trainer_id =
    create_branch_staff("Cascade Trainer", "ct@t.com", "0171111112", "RenameOld", "cascadetrainer", "hash", TRAINER);
  id_t member_id = create_gym_member(
    "Cascade Member", "cmem@t.com", "0172222222", "RenameOld", "cascademember", "hash", plan, MEMBERSHIP_ON_HOLD
  );
  id_t outsider_id = create_gym_member(
    "Cascade Outsider", "co@t.com", "0173333333", "RenameOther", "cascadeoutsider", "hash", plan, MEMBERSHIP_ON_HOLD
  );
  assert(manager_id != 0 && trainer_id != 0 && member_id != 0 && outsider_id != 0);

  assert(update_branch_name("RenameOld", "RenameNew") == true);

  branch_staff_t manager;
  branch_staff_t trainer;
  gym_member_t member;
  gym_member_t outsider;
  assert(get_branch_staff_by_id(manager_id, &manager) == true);
  assert(get_branch_staff_by_id(trainer_id, &trainer) == true);
  assert(get_gym_member_by_id(member_id, &member) == true);
  assert(get_gym_member_by_id(outsider_id, &outsider) == true);
  assert(strcmp(manager.gym_branch, "RenameNew") == 0);
  assert(strcmp(trainer.gym_branch, "RenameNew") == 0);
  assert(strcmp(member.gym_branch, "RenameNew") == 0);
  assert(strcmp(outsider.gym_branch, "RenameOther") == 0);

  // Branch-scoped counts must follow the renamed records.
  assert(branch_manager_count("RenameNew") == 1);
  assert(branch_trainer_count("RenameNew") == 1);
  assert(branch_member_count("RenameNew") == 1);
  assert(branch_member_count("RenameOld") == 0);

  // Reload everything from disk to verify the cascade persisted.
  load_branches();
  load_branch_staff();
  load_gym_members();
  assert(find_branch("RenameNew") != -1);
  assert(find_branch("RenameOld") == -1);
  assert(get_branch_staff_by_id(manager_id, &manager) == true);
  assert(get_gym_member_by_id(member_id, &member) == true);
  assert(strcmp(manager.gym_branch, "RenameNew") == 0);
  assert(strcmp(member.gym_branch, "RenameNew") == 0);

  // Remove created users so later user-table tests start from empty files.
  assert(delete_branch_staff(manager_id) == true);
  assert(delete_branch_staff(trainer_id) == true);
  assert(delete_gym_member(member_id) == true);
  assert(delete_gym_member(outsider_id) == true);
}

/**
 * Verifies that update_branch_name rejects missing, empty, unknown old, and
 * taken new names without touching any existing branch.
 */
void test_update_branch_name_rejects_invalid_names()
{
  cleanup_branches_file();
  load_branches();

  add_branch("KeepA");
  add_branch("KeepB");

  assert(update_branch_name(NULL, "Anything") == false);
  assert(update_branch_name("KeepA", NULL) == false);
  assert(update_branch_name("", "Anything") == false);
  assert(update_branch_name("KeepA", "") == false);
  assert(update_branch_name("NoSuchBranch", "Anything") == false);
  assert(update_branch_name("KeepA", "KeepB") == false);
  assert(update_branch_name("KeepA", "KeepA") == false);

  assert(get_branch_count() == 2);
  assert(find_branch("KeepA") != -1);
  assert(find_branch("KeepB") != -1);
  assert(find_branch("Anything") == -1);
}

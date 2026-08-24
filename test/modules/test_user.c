#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "../../src/modules/user.h"
#include "../../src/settings.h"
#include "../../src/types.h"
#include "../../src/utils/datetime_utils.h"
#include "test_user.h"

/**
 * Removes all three user data files from the test_data directory.
 * Called once at startup from test_main, not between individual tests.
 */
void cleanup_user_files()
{
  remove(SYSADMINS_FILE_PATH);
  remove(BRANCH_STAFF_FILE_PATH);
  remove(GYM_MEMBERS_FILE_PATH);
}

// ---- sysadmin tests ----

void test_create_sysadmin_and_get()
{
  load_sysadmins();
  load_branch_staff();
  load_gym_members();

  id_t id = create_sysadmin("admin", "storedhash123");
  assert(id == 1);

  sysadmin_t found;
  assert(get_sysadmin_by_id(id, &found) == true);
  assert(found.id == 1);
  assert(strcmp(found.username, "admin") == 0);
  assert(strcmp(found.password_hash, "storedhash123") == 0);
}

void test_create_sysadmin_rejects_second()
{
  load_sysadmins();
  load_branch_staff();
  load_gym_members();

  id_t second = create_sysadmin("admin2", "hash2");
  assert(second == 0);
}

void test_create_sysadmin_rejects_empty_username()
{
  id_t id = create_sysadmin("", "hash");
  assert(id == 0);
}

void test_create_sysadmin_rejects_null_username()
{
  id_t id = create_sysadmin(NULL, "hash");
  assert(id == 0);
}

void test_create_sysadmin_rejects_empty_password()
{
  id_t id = create_sysadmin("admin2", "");
  assert(id == 0);
}

void test_get_sysadmin_by_username()
{
  sysadmin_t found;
  assert(get_sysadmin_by_username("admin", &found) == true);
  assert(strcmp(found.username, "admin") == 0);

  assert(get_sysadmin_by_username("nobody", &found) == false);
}

void test_get_sysadmin_by_username_null()
{
  sysadmin_t found;
  assert(get_sysadmin_by_username(NULL, &found) == false);
  assert(get_sysadmin_by_username("", &found) == false);
  assert(get_sysadmin_by_username("admin", NULL) == false);
}

void test_get_sysadmin_by_id_not_found()
{
  sysadmin_t found;
  assert(get_sysadmin_by_id(999, &found) == false);
}

void test_load_sysadmins_roundtrip()
{
  load_sysadmins();

  sysadmin_t found;
  assert(get_sysadmin_by_username("admin", &found) == true);
  assert(strcmp(found.password_hash, "storedhash123") == 0);
}

// ---- branch staff tests ----

void test_create_branch_staff_and_get()
{
  cleanup_user_files();
  load_sysadmins();
  load_branch_staff();
  load_gym_members();

  id_t id = create_branch_staff("Rahim Uddin", "rahim@test.com", "0171234567", "Dhanmondi", "rahim", "hash1", TRAINER);
  assert(id == 1);

  branch_staff_t found;
  assert(get_branch_staff_by_id(id, &found) == true);
  assert(found.id == 1);
  assert(strcmp(found.full_name, "Rahim Uddin") == 0);
  assert(strcmp(found.email, "rahim@test.com") == 0);
  assert(strcmp(found.phone_number, "0171234567") == 0);
  assert(strcmp(found.gym_branch, "Dhanmondi") == 0);
  assert(strcmp(found.username, "rahim") == 0);
  assert(strcmp(found.password_hash, "hash1") == 0);
  assert(found.role == TRAINER);
}

void test_create_branch_staff_auto_increment_id()
{
  id_t id1 = create_branch_staff("A", "a@t.com", "0171111111", "Branch1", "user1", "h1", TRAINER);
  id_t id2 = create_branch_staff("B", "b@t.com", "0172222222", "Branch1", "user2", "h2", BRANCH_MANAGER);
  assert(id1 == 2);
  assert(id2 == 3);
  assert(id2 > id1);
}

void test_create_branch_staff_rejects_duplicate_username()
{
  id_t id2 = create_branch_staff("C", "c@t.com", "0173333333", "Branch1", "rahim", "h2", TRAINER);
  assert(id2 == 0);
}

void test_create_branch_staff_rejects_empty_fields()
{
  assert(create_branch_staff("", "a@t.com", "0171111111", "Branch1", "u10", "h1", TRAINER) == 0);
  assert(create_branch_staff("Name", "", "0171111111", "Branch1", "u11", "h1", TRAINER) == 0);
  assert(create_branch_staff("Name", "a@t.com", "", "Branch1", "u12", "h1", TRAINER) == 0);
  assert(create_branch_staff("Name", "a@t.com", "0171111111", "", "u13", "h1", TRAINER) == 0);
  assert(create_branch_staff("Name", "a@t.com", "0171111111", "Branch1", "", "h1", TRAINER) == 0);
  assert(create_branch_staff("Name", "a@t.com", "0171111111", "Branch1", "u14", "", TRAINER) == 0);
}

void test_get_branch_staff_by_username()
{
  branch_staff_t found;
  assert(get_branch_staff_by_username("rahim", &found) == true);
  assert(strcmp(found.gym_branch, "Dhanmondi") == 0);
  assert(found.role == TRAINER);

  assert(get_branch_staff_by_username("nobody", &found) == false);
}

void test_get_branch_staff_by_id_not_found()
{
  branch_staff_t found;
  assert(get_branch_staff_by_id(999, &found) == false);
}

void test_load_branch_staff_roundtrip()
{
  cleanup_user_files();
  load_sysadmins();
  load_branch_staff();
  load_gym_members();

  create_branch_staff("Test", "t@t.com", "0171111111", "Gulshan", "tester", "hashval", BRANCH_MANAGER);

  load_branch_staff();
  branch_staff_t found;
  assert(get_branch_staff_by_username("tester", &found) == true);
  assert(found.role == BRANCH_MANAGER);
  assert(strcmp(found.password_hash, "hashval") == 0);
}

// ---- gym member tests ----

void test_create_gym_member_and_get()
{
  cleanup_user_files();
  load_sysadmins();
  load_branch_staff();
  load_gym_members();

  subscription_plan_t plan;
  plan.payable_amount = 1500;
  plan.interval_days = 30;

  id_t id = create_gym_member(
    "Nusrat Jahan", "nusrat@test.com", "0181234567", "Uttara", "nusrat", "hash1", plan, MEMBERSHIP_ON_HOLD
  );
  assert(id == 1);

  gym_member_t found;
  assert(get_gym_member_by_id(id, &found) == true);
  assert(found.id == 1);
  assert(strcmp(found.full_name, "Nusrat Jahan") == 0);
  assert(strcmp(found.email, "nusrat@test.com") == 0);
  assert(strcmp(found.phone_number, "0181234567") == 0);
  assert(strcmp(found.gym_branch, "Uttara") == 0);
  assert(strcmp(found.username, "nusrat") == 0);
  assert(found.plan.payable_amount == 1500);
  assert(found.plan.interval_days == 30);
  assert(found.status == MEMBERSHIP_ON_HOLD);
  assert(found.due_amount == 0);
  assert(is_empty_datetime(found.last_payment_date));
}

void test_create_gym_member_auto_increment_id()
{
  subscription_plan_t plan;
  plan.payable_amount = 1000;
  plan.interval_days = 30;

  id_t id1 = create_gym_member("A", "a@t.com", "0171111111", "B1", "u1", "h1", plan, MEMBERSHIP_ON_HOLD);
  id_t id2 = create_gym_member("B", "b@t.com", "0172222222", "B1", "u2", "h2", plan, MEMBERSHIP_ACTIVE);
  assert(id1 == 2);
  assert(id2 == 3);
}

void test_create_gym_member_rejects_duplicate_username()
{
  subscription_plan_t plan;
  plan.payable_amount = 1000;
  plan.interval_days = 30;

  id_t id2 = create_gym_member("C", "c@t.com", "0173333333", "B1", "nusrat", "h2", plan, MEMBERSHIP_ACTIVE);
  assert(id2 == 0);
}

void test_create_gym_member_rejects_empty_fields()
{
  subscription_plan_t plan;
  plan.payable_amount = 1000;
  plan.interval_days = 30;

  assert(create_gym_member("", "a@t.com", "0171111111", "B1", "u10", "h1", plan, MEMBERSHIP_ON_HOLD) == 0);
  assert(create_gym_member("Name", "", "0171111111", "B1", "u11", "h1", plan, MEMBERSHIP_ON_HOLD) == 0);
  assert(create_gym_member("Name", "a@t.com", "", "B1", "u12", "h1", plan, MEMBERSHIP_ON_HOLD) == 0);
  assert(create_gym_member("Name", "a@t.com", "0171111111", "", "u13", "h1", plan, MEMBERSHIP_ON_HOLD) == 0);
  assert(create_gym_member("Name", "a@t.com", "0171111111", "B1", "", "h1", plan, MEMBERSHIP_ON_HOLD) == 0);
  assert(create_gym_member("Name", "a@t.com", "0171111111", "B1", "u14", "", plan, MEMBERSHIP_ON_HOLD) == 0);
}

void test_get_gym_member_by_username()
{
  gym_member_t found;
  assert(get_gym_member_by_username("nusrat", &found) == true);
  assert(strcmp(found.gym_branch, "Uttara") == 0);
  assert(found.status == MEMBERSHIP_ON_HOLD);

  assert(get_gym_member_by_username("nobody", &found) == false);
}

void test_get_gym_member_by_id_not_found()
{
  gym_member_t found;
  assert(get_gym_member_by_id(999, &found) == false);
}

void test_load_gym_members_roundtrip()
{
  cleanup_user_files();
  load_sysadmins();
  load_branch_staff();
  load_gym_members();

  subscription_plan_t plan;
  plan.payable_amount = 1500;
  plan.interval_days = 30;

  create_gym_member("Test", "t@t.com", "0171111111", "Banani", "tester_m", "hashval", plan, MEMBERSHIP_ACTIVE);

  load_gym_members();
  gym_member_t found;
  assert(get_gym_member_by_username("tester_m", &found) == true);
  assert(found.plan.payable_amount == 1500);
  assert(found.plan.interval_days == 30);
  assert(strcmp(found.password_hash, "hashval") == 0);
}

// ---- username_exists tests ----

void test_username_exists_returns_false_when_empty()
{
  cleanup_user_files();
  load_sysadmins();
  load_branch_staff();
  load_gym_members();

  assert(username_exists("anyone") == false);
}

void test_username_exists_finds_sysadmin()
{
  create_sysadmin("superadmin", "hash");
  assert(username_exists("superadmin") == true);
}

void test_username_exists_finds_branch_staff()
{
  create_branch_staff("Name", "e@t.com", "0171111111", "B1", "staff_user", "hash", TRAINER);
  assert(username_exists("staff_user") == true);
}

void test_username_exists_finds_gym_member()
{
  subscription_plan_t plan;
  plan.payable_amount = 1000;
  plan.interval_days = 30;

  create_gym_member("Name", "e@t.com", "0171111111", "B1", "member_user", "hash", plan, MEMBERSHIP_ON_HOLD);
  assert(username_exists("member_user") == true);
}

void test_username_exists_cross_table_uniqueness()
{
  subscription_plan_t plan;
  plan.payable_amount = 1000;
  plan.interval_days = 30;

  id_t staff_id = create_branch_staff("Name", "e@t.com", "0171111111", "B1", "superadmin", "hash2", TRAINER);
  assert(staff_id == 0);

  id_t member_id =
    create_gym_member("Name", "e@t.com", "0171111111", "B1", "superadmin", "hash3", plan, MEMBERSHIP_ON_HOLD);
  assert(member_id == 0);
}

void test_username_exists_null()
{
  assert(username_exists(NULL) == false);
}

// ---- branch count tests ----

void test_branch_manager_count_zero_when_empty()
{
  cleanup_user_files();
  load_sysadmins();
  load_branch_staff();
  load_gym_members();

  assert(branch_manager_count("Dhanmondi") == 0);
}

void test_branch_manager_count_one()
{
  create_branch_staff("M1", "m1@t.com", "0171111111", "Dhanmondi", "mgr1", "h1", BRANCH_MANAGER);
  assert(branch_manager_count("Dhanmondi") == 1);
}

void test_branch_manager_count_ignores_trainers()
{
  create_branch_staff("T1", "t1@t.com", "0172222222", "Dhanmondi", "tr1", "h1", TRAINER);
  create_branch_staff("T2", "t2@t.com", "0173333333", "Dhanmondi", "tr2", "h2", TRAINER);
  assert(branch_manager_count("Dhanmondi") == 1);
}

void test_branch_manager_count_ignores_other_branches()
{
  create_branch_staff("M2", "m2@t.com", "0174444444", "Banani", "mgr_b", "h1", BRANCH_MANAGER);
  assert(branch_manager_count("Dhanmondi") == 1);
}

void test_branch_trainer_count_zero_when_empty()
{
  cleanup_user_files();
  load_sysadmins();
  load_branch_staff();
  load_gym_members();

  assert(branch_trainer_count("Dhanmondi") == 0);
}

void test_branch_trainer_count_multiple()
{
  create_branch_staff("T1", "t1@t.com", "0171111111", "Dhanmondi", "tr1", "h1", TRAINER);
  create_branch_staff("T2", "t2@t.com", "0172222222", "Dhanmondi", "tr2", "h2", TRAINER);
  create_branch_staff("T3", "t3@t.com", "0173333333", "Dhanmondi", "tr3", "h3", TRAINER);
  assert(branch_trainer_count("Dhanmondi") == 3);
}

void test_branch_trainer_count_ignores_managers()
{
  create_branch_staff("M1", "m1@t.com", "0174444444", "Dhanmondi", "mgr1", "h1", BRANCH_MANAGER);
  assert(branch_trainer_count("Dhanmondi") == 3);
}

void test_branch_member_count_zero_when_empty()
{
  cleanup_user_files();
  load_sysadmins();
  load_branch_staff();
  load_gym_members();

  assert(branch_member_count("Dhanmondi") == 0);
}

void test_branch_member_count_multiple()
{
  subscription_plan_t plan;
  plan.payable_amount = 1000;
  plan.interval_days = 30;

  create_gym_member("A", "a@t.com", "0171111111", "Dhanmondi", "m1", "h1", plan, MEMBERSHIP_ON_HOLD);
  create_gym_member("B", "b@t.com", "0172222222", "Dhanmondi", "m2", "h2", plan, MEMBERSHIP_ACTIVE);
  assert(branch_member_count("Dhanmondi") == 2);
}

void test_branch_member_count_ignores_other_branches()
{
  subscription_plan_t plan;
  plan.payable_amount = 1000;
  plan.interval_days = 30;

  create_gym_member("C", "c@t.com", "0173333333", "Banani", "m3", "h3", plan, MEMBERSHIP_ON_HOLD);
  assert(branch_member_count("Dhanmondi") == 2);
}

void test_branch_counts_null_and_empty()
{
  assert(branch_manager_count(NULL) == 0);
  assert(branch_manager_count("") == 0);
  assert(branch_trainer_count(NULL) == 0);
  assert(branch_trainer_count("") == 0);
  assert(branch_member_count(NULL) == 0);
  assert(branch_member_count("") == 0);
}

// ---- cross-table username uniqueness ----

void test_staff_username_blocks_member_creation()
{
  cleanup_user_files();
  load_sysadmins();
  load_branch_staff();
  load_gym_members();

  create_branch_staff("Name", "e@t.com", "0171111111", "B1", "shared", "h1", TRAINER);

  subscription_plan_t plan;
  plan.payable_amount = 1000;
  plan.interval_days = 30;

  id_t id = create_gym_member("Name", "e@t.com", "0171111111", "B1", "shared", "h2", plan, MEMBERSHIP_ON_HOLD);
  assert(id == 0);
}

void test_member_username_blocks_staff_creation()
{
  cleanup_user_files();
  load_sysadmins();
  load_branch_staff();
  load_gym_members();

  subscription_plan_t plan;
  plan.payable_amount = 1000;
  plan.interval_days = 30;

  create_gym_member("Name", "e@t.com", "0171111111", "B1", "taken", "h1", plan, MEMBERSHIP_ON_HOLD);

  id_t id = create_branch_staff("Name", "e@t.com", "0171111111", "B1", "taken", "h2", TRAINER);
  assert(id == 0);
}

// ---- deletion ----

/**
 * Verifies that delete_branch_staff removes the record from memory and
 * disk and frees the username for reuse.
 */
void test_delete_branch_staff_removes_and_persists()
{
  cleanup_user_files();
  load_sysadmins();
  load_branch_staff();
  load_gym_members();

  id_t id1 = create_branch_staff("Staff One", "s1@t.com", "0171111111", "B1", "staffdel1", "h1", TRAINER);
  id_t id2 = create_branch_staff("Staff Two", "s2@t.com", "0172222222", "B1", "staffdel2", "h2", BRANCH_MANAGER);
  assert(id1 != 0 && id2 != 0);

  branch_staff_t found;
  assert(delete_branch_staff(id1) == true);
  assert(get_branch_staff_by_id(id1, &found) == false);
  assert(get_branch_staff_by_username("staffdel1", &found) == false);
  assert(get_branch_staff_by_id(id2, &found) == true);

  load_branch_staff();
  assert(get_branch_staff_by_id(id1, &found) == false);
  assert(get_branch_staff_by_id(id2, &found) == true);

  id_t reused = create_branch_staff("Staff Three", "s3@t.com", "0173333333", "B1", "staffdel1", "h3", TRAINER);
  assert(reused != 0);
}

/**
 * Verifies that delete_branch_staff rejects an unknown id.
 */
void test_delete_branch_staff_rejects_unknown_id()
{
  cleanup_user_files();
  load_sysadmins();
  load_branch_staff();
  load_gym_members();

  assert(delete_branch_staff(9999) == false);
}

/**
 * Verifies that delete_gym_member removes the record from memory and disk
 * and frees the username for reuse.
 */
void test_delete_gym_member_removes_and_persists()
{
  cleanup_user_files();
  load_sysadmins();
  load_branch_staff();
  load_gym_members();

  subscription_plan_t plan;
  plan.payable_amount = 1000;
  plan.interval_days = 30;

  id_t id1 =
    create_gym_member("Member One", "m1@t.com", "0181111111", "B1", "memberdel1", "h1", plan, MEMBERSHIP_ACTIVE);
  id_t id2 =
    create_gym_member("Member Two", "m2@t.com", "0182222222", "B1", "memberdel2", "h2", plan, MEMBERSHIP_ON_HOLD);
  assert(id1 != 0 && id2 != 0);

  gym_member_t found;
  assert(delete_gym_member(id1) == true);
  assert(get_gym_member_by_id(id1, &found) == false);
  assert(get_gym_member_by_username("memberdel1", &found) == false);
  assert(get_gym_member_by_id(id2, &found) == true);

  load_gym_members();
  assert(get_gym_member_by_id(id1, &found) == false);
  assert(get_gym_member_by_id(id2, &found) == true);

  subscription_plan_t reuse_plan;
  reuse_plan.payable_amount = 500;
  reuse_plan.interval_days = 15;

  id_t reused = create_gym_member(
    "Member Three", "m3@t.com", "0183333333", "B1", "memberdel1", "h3", reuse_plan, MEMBERSHIP_ON_HOLD
  );
  assert(reused != 0);
}

/**
 * Verifies that delete_gym_member rejects a member with outstanding dues.
 *
 * The indebted record is written directly to the data file because creation
 * always starts members with zero dues.
 */
void test_delete_gym_member_rejects_member_with_dues()
{
  cleanup_user_files();
  load_sysadmins();
  load_branch_staff();
  load_gym_members();

  FILE *file = fopen(GYM_MEMBERS_FILE_PATH, "w");
  assert(file != NULL);
  fprintf(file, "1|Debtor Member|debt@t.com|0184444444|B1|debtor|h1|1704067200|0|750|1000|30|%d\n", MEMBERSHIP_ACTIVE);
  fclose(file);

  assert(load_gym_members() == 1);

  gym_member_t found;
  assert(delete_gym_member(1) == false);
  assert(get_gym_member_by_id(1, &found) == true);
  assert(found.due_amount == 750);
}

// ---- update tests ----

void test_update_branch_staff_updates_fields_and_persists()
{
  cleanup_user_files();
  load_sysadmins();
  load_branch_staff();
  load_gym_members();

  id_t id = create_branch_staff("Old Name", "old@test.com", "0171111111", "B1", "updater1", "h1", TRAINER);
  assert(id != 0);

  assert(update_branch_staff(id, "New Name", "new@test.com", "0172222222") == true);

  branch_staff_t found;
  assert(update_branch_staff(id, "New Name", "new@test.com", "0172222222") == true);

  assert(get_branch_staff_by_id(id, &found) == true);
  assert(strcmp(found.full_name, "New Name") == 0);
  assert(strcmp(found.email, "new@test.com") == 0);
  assert(strcmp(found.phone_number, "0172222222") == 0);

  // Verify persistence.
  load_branch_staff();
  assert(get_branch_staff_by_id(id, &found) == true);
  assert(strcmp(found.full_name, "New Name") == 0);
}

void test_update_branch_staff_rejects_unknown_id()
{
  cleanup_user_files();
  load_sysadmins();
  load_branch_staff();
  load_gym_members();

  assert(update_branch_staff(9999, "Name", "e@t.com", "0171111111") == false);
}

void test_update_branch_staff_rejects_empty_fields()
{
  cleanup_user_files();
  load_sysadmins();
  load_branch_staff();
  load_gym_members();

  id_t id = create_branch_staff("Name", "e@t.com", "0171111111", "B1", "updater2", "h1", TRAINER);
  assert(id != 0);

  assert(update_branch_staff(id, "", "e@t.com", "0171111111") == false);
  assert(update_branch_staff(id, "Name", "", "0171111111") == false);
  assert(update_branch_staff(id, "Name", "e@t.com", "") == false);
}

void test_update_gym_member_updates_fields_and_persists()
{
  cleanup_user_files();
  load_sysadmins();
  load_branch_staff();
  load_gym_members();

  subscription_plan_t plan;
  plan.payable_amount = 1000;
  plan.interval_days = 30;

  id_t id =
    create_gym_member("Old Name", "old@test.com", "0181111111", "B1", "mupdater1", "h1", plan, MEMBERSHIP_ACTIVE);
  assert(id != 0);

  assert(update_gym_member(id, "New Name", "new@test.com", "0182222222", "B2", "mupdater1_new") == true);

  gym_member_t found;
  assert(update_gym_member(id, "New Name", "new@test.com", "0182222222", "B2", "mupdater1_new") == true);

  assert(get_gym_member_by_id(id, &found) == true);
  assert(strcmp(found.full_name, "New Name") == 0);
  assert(strcmp(found.email, "new@test.com") == 0);
  assert(strcmp(found.phone_number, "0182222222") == 0);
  assert(strcmp(found.gym_branch, "B2") == 0);
  assert(strcmp(found.username, "mupdater1_new") == 0);

  // Verify persistence.
  load_gym_members();
  assert(get_gym_member_by_id(id, &found) == true);
  assert(strcmp(found.full_name, "New Name") == 0);
}

void test_update_gym_member_rejects_unknown_id()
{
  cleanup_user_files();
  load_sysadmins();
  load_branch_staff();
  load_gym_members();

  assert(update_gym_member(9999, "Name", "e@t.com", "0171111111", "B1", "user") == false);
}

void test_update_gym_member_rejects_empty_fields()
{
  cleanup_user_files();
  load_sysadmins();
  load_branch_staff();
  load_gym_members();

  subscription_plan_t plan;
  plan.payable_amount = 1000;
  plan.interval_days = 30;

  id_t id = create_gym_member("Name", "e@t.com", "0171111111", "B1", "mupdater2", "h1", plan, MEMBERSHIP_ON_HOLD);
  assert(id != 0);

  assert(update_gym_member(id, "", "e@t.com", "0171111111", "B1", "mupdater2") == false);
  assert(update_gym_member(id, "Name", "", "0171111111", "B1", "mupdater2") == false);
  assert(update_gym_member(id, "Name", "e@t.com", "", "B1", "mupdater2") == false);
  assert(update_gym_member(id, "Name", "e@t.com", "0171111111", "", "mupdater2") == false);
  assert(update_gym_member(id, "Name", "e@t.com", "0171111111", "B1", "") == false);
}

void test_update_gym_member_rejects_duplicate_username()
{
  cleanup_user_files();
  load_sysadmins();
  load_branch_staff();
  load_gym_members();

  subscription_plan_t plan;
  plan.payable_amount = 1000;
  plan.interval_days = 30;

  create_gym_member("A", "a@t.com", "0171111111", "B1", "taken", "h1", plan, MEMBERSHIP_ON_HOLD);
  id_t id2 = create_gym_member("B", "b@t.com", "0172222222", "B1", "other", "h2", plan, MEMBERSHIP_ON_HOLD);
  assert(id2 != 0);

  assert(update_gym_member(id2, "B", "b@t.com", "0172222222", "B1", "taken") == false);

  // Original username unchanged.
  gym_member_t found;
  assert(get_gym_member_by_id(id2, &found) == true);
  assert(strcmp(found.username, "other") == 0);
}

void test_update_gym_member_allows_same_username()
{
  cleanup_user_files();
  load_sysadmins();
  load_branch_staff();
  load_gym_members();

  subscription_plan_t plan;
  plan.payable_amount = 1000;
  plan.interval_days = 30;

  id_t id = create_gym_member("Name", "e@t.com", "0171111111", "B1", "keeper", "h1", plan, MEMBERSHIP_ACTIVE);
  assert(id != 0);

  // Updating other fields while keeping the same username should succeed.
  assert(update_gym_member(id, "New Name", "new@t.com", "0172222222", "B2", "keeper") == true);

  gym_member_t found;
  assert(get_gym_member_by_id(id, &found) == true);
  assert(strcmp(found.username, "keeper") == 0);
  assert(strcmp(found.full_name, "New Name") == 0);
}

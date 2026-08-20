#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "../../src/modules/user.h"
#include "../../src/settings.h"
#include "../../src/types.h"
#include "../../src/utils/file_util.h"
#include "test_user.h"

static void cleanup_user_files()
{
  char path[PATH_BUFFER_SIZE];
  build_file_path(SYSDADMINS_FILENAME, path, PATH_BUFFER_SIZE);
  remove(path);
  build_file_path(BRANCH_STAFF_FILENAME, path, PATH_BUFFER_SIZE);
  remove(path);
  build_file_path(GYM_MEMBERS_FILENAME, path, PATH_BUFFER_SIZE);
  remove(path);
}

// ---- sysadmin tests ----

static void test_create_sysadmin_and_get()
{
  cleanup_user_files();
  load_sysadmins();
  load_branch_staff();
  load_gym_members();

  id_t id = create_sysadmin("admin", "storedhash123");
  assert(id == 1);

  sysadmin_t *found = get_sysadmin_by_id(id);
  assert(found != NULL);
  assert(found->id == 1);
  assert(strcmp(found->username, "admin") == 0);
  assert(strcmp(found->password_hash, "storedhash123") == 0);

  cleanup_user_files();
}

static void test_create_sysadmin_rejects_second()
{
  cleanup_user_files();
  load_sysadmins();
  load_branch_staff();
  load_gym_members();

  create_sysadmin("admin", "hash1");
  id_t second = create_sysadmin("admin2", "hash2");
  assert(second == 0);

  cleanup_user_files();
}

static void test_create_sysadmin_rejects_empty_username()
{
  cleanup_user_files();
  load_sysadmins();
  load_branch_staff();
  load_gym_members();

  id_t id = create_sysadmin("", "hash");
  assert(id == 0);

  cleanup_user_files();
}

static void test_create_sysadmin_rejects_null_username()
{
  cleanup_user_files();
  load_sysadmins();
  load_branch_staff();
  load_gym_members();

  id_t id = create_sysadmin(NULL, "hash");
  assert(id == 0);

  cleanup_user_files();
}

static void test_create_sysadmin_rejects_empty_password()
{
  cleanup_user_files();
  load_sysadmins();
  load_branch_staff();
  load_gym_members();

  id_t id = create_sysadmin("admin", "");
  assert(id == 0);

  cleanup_user_files();
}

static void test_get_sysadmin_by_username()
{
  cleanup_user_files();
  load_sysadmins();
  load_branch_staff();
  load_gym_members();

  create_sysadmin("root", "hash123");
  sysadmin_t *found = get_sysadmin_by_username("root");
  assert(found != NULL);
  assert(strcmp(found->username, "root") == 0);

  assert(get_sysadmin_by_username("nobody") == NULL);

  cleanup_user_files();
}

static void test_get_sysadmin_by_username_null()
{
  cleanup_user_files();
  load_sysadmins();
  load_branch_staff();
  load_gym_members();

  assert(get_sysadmin_by_username(NULL) == NULL);

  cleanup_user_files();
}

static void test_get_sysadmin_by_id_not_found()
{
  cleanup_user_files();
  load_sysadmins();
  load_branch_staff();
  load_gym_members();

  assert(get_sysadmin_by_id(999) == NULL);

  cleanup_user_files();
}

static void test_load_sysadmins_roundtrip()
{
  cleanup_user_files();
  load_sysadmins();
  load_branch_staff();
  load_gym_members();

  create_sysadmin("admin", "hash_a");

  load_sysadmins();

  sysadmin_t *found = get_sysadmin_by_username("admin");
  assert(found != NULL);
  assert(strcmp(found->password_hash, "hash_a") == 0);

  cleanup_user_files();
}

// ---- branch staff tests ----

static void test_create_branch_staff_and_get()
{
  cleanup_user_files();
  load_sysadmins();
  load_branch_staff();
  load_gym_members();

  id_t id = create_branch_staff("Rahim Uddin", "rahim@test.com", "0171234567", "Dhanmondi", "rahim", "hash1", TRAINER);
  assert(id == 1);

  branch_staff_t *found = get_branch_staff_by_id(id);
  assert(found != NULL);
  assert(found->id == 1);
  assert(strcmp(found->full_name, "Rahim Uddin") == 0);
  assert(strcmp(found->email, "rahim@test.com") == 0);
  assert(strcmp(found->phone_number, "0171234567") == 0);
  assert(strcmp(found->gym_branch, "Dhanmondi") == 0);
  assert(strcmp(found->username, "rahim") == 0);
  assert(strcmp(found->password_hash, "hash1") == 0);
  assert(found->role == TRAINER);

  cleanup_user_files();
}

static void test_create_branch_staff_auto_increment_id()
{
  cleanup_user_files();
  load_sysadmins();
  load_branch_staff();
  load_gym_members();

  id_t id1 = create_branch_staff("A", "a@t.com", "0171111111", "Branch1", "user1", "h1", TRAINER);
  id_t id2 = create_branch_staff("B", "b@t.com", "0172222222", "Branch1", "user2", "h2", BRANCH_MANAGER);
  assert(id1 == 1);
  assert(id2 == 2);
  assert(id2 > id1);

  cleanup_user_files();
}

static void test_create_branch_staff_rejects_duplicate_username()
{
  cleanup_user_files();
  load_sysadmins();
  load_branch_staff();
  load_gym_members();

  create_branch_staff("A", "a@t.com", "0171111111", "Branch1", "same_user", "h1", TRAINER);
  id_t id2 = create_branch_staff("B", "b@t.com", "0172222222", "Branch1", "same_user", "h2", TRAINER);
  assert(id2 == 0);

  cleanup_user_files();
}

static void test_create_branch_staff_rejects_empty_fields()
{
  cleanup_user_files();
  load_sysadmins();
  load_branch_staff();
  load_gym_members();

  assert(create_branch_staff("", "a@t.com", "0171111111", "Branch1", "u1", "h1", TRAINER) == 0);
  assert(create_branch_staff("Name", "", "0171111111", "Branch1", "u2", "h1", TRAINER) == 0);
  assert(create_branch_staff("Name", "a@t.com", "", "Branch1", "u3", "h1", TRAINER) == 0);
  assert(create_branch_staff("Name", "a@t.com", "0171111111", "", "u4", "h1", TRAINER) == 0);
  assert(create_branch_staff("Name", "a@t.com", "0171111111", "Branch1", "", "h1", TRAINER) == 0);
  assert(create_branch_staff("Name", "a@t.com", "0171111111", "Branch1", "u5", "", TRAINER) == 0);

  cleanup_user_files();
}

static void test_get_branch_staff_by_username()
{
  cleanup_user_files();
  load_sysadmins();
  load_branch_staff();
  load_gym_members();

  create_branch_staff("Karim", "k@t.com", "0171111111", "Banani", "karim", "hash", TRAINER);
  branch_staff_t *found = get_branch_staff_by_username("karim");
  assert(found != NULL);
  assert(strcmp(found->gym_branch, "Banani") == 0);
  assert(found->role == TRAINER);

  assert(get_branch_staff_by_username("nobody") == NULL);

  cleanup_user_files();
}

static void test_get_branch_staff_by_id_not_found()
{
  cleanup_user_files();
  load_sysadmins();
  load_branch_staff();
  load_gym_members();

  assert(get_branch_staff_by_id(999) == NULL);

  cleanup_user_files();
}

static void test_load_branch_staff_roundtrip()
{
  cleanup_user_files();
  load_sysadmins();
  load_branch_staff();
  load_gym_members();

  create_branch_staff("Test", "t@t.com", "0171111111", "Gulshan", "tester", "hashval", BRANCH_MANAGER);

  load_branch_staff();
  branch_staff_t *found = get_branch_staff_by_username("tester");
  assert(found != NULL);
  assert(found->role == BRANCH_MANAGER);
  assert(strcmp(found->password_hash, "hashval") == 0);

  cleanup_user_files();
}

// ---- gym member tests ----

static void test_create_gym_member_and_get()
{
  cleanup_user_files();
  load_sysadmins();
  load_branch_staff();
  load_gym_members();

  subscription_plan_t plan;
  plan.payable_amount = 1500;
  plan.interval_days = 30;

  id_t id = create_gym_member("Nusrat Jahan", "nusrat@test.com", "0181234567", "Uttara", "nusrat", "hash1", plan,
                              MEMBERSHIP_ON_HOLD);
  assert(id == 1);

  gym_member_t *found = get_gym_member_by_id(id);
  assert(found != NULL);
  assert(found->id == 1);
  assert(strcmp(found->full_name, "Nusrat Jahan") == 0);
  assert(strcmp(found->email, "nusrat@test.com") == 0);
  assert(strcmp(found->phone_number, "0181234567") == 0);
  assert(strcmp(found->gym_branch, "Uttara") == 0);
  assert(strcmp(found->username, "nusrat") == 0);
  assert(found->plan.payable_amount == 1500);
  assert(found->plan.interval_days == 30);
  assert(found->status == MEMBERSHIP_ON_HOLD);
  assert(found->due_amount == 0);
  assert(found->last_payment_date == 0);

  cleanup_user_files();
}

static void test_create_gym_member_auto_increment_id()
{
  cleanup_user_files();
  load_sysadmins();
  load_branch_staff();
  load_gym_members();

  subscription_plan_t plan;
  plan.payable_amount = 1000;
  plan.interval_days = 30;

  id_t id1 = create_gym_member("A", "a@t.com", "0171111111", "B1", "u1", "h1", plan, MEMBERSHIP_ON_HOLD);
  id_t id2 = create_gym_member("B", "b@t.com", "0172222222", "B1", "u2", "h2", plan, MEMBERSHIP_ACTIVE);
  assert(id1 == 1);
  assert(id2 == 2);

  cleanup_user_files();
}

static void test_create_gym_member_rejects_duplicate_username()
{
  cleanup_user_files();
  load_sysadmins();
  load_branch_staff();
  load_gym_members();

  subscription_plan_t plan;
  plan.payable_amount = 1000;
  plan.interval_days = 30;

  create_gym_member("A", "a@t.com", "0171111111", "B1", "same_user", "h1", plan, MEMBERSHIP_ON_HOLD);
  id_t id2 = create_gym_member("B", "b@t.com", "0172222222", "B1", "same_user", "h2", plan, MEMBERSHIP_ACTIVE);
  assert(id2 == 0);

  cleanup_user_files();
}

static void test_create_gym_member_rejects_empty_fields()
{
  cleanup_user_files();
  load_sysadmins();
  load_branch_staff();
  load_gym_members();

  subscription_plan_t plan;
  plan.payable_amount = 1000;
  plan.interval_days = 30;

  assert(create_gym_member("", "a@t.com", "0171111111", "B1", "u1", "h1", plan, MEMBERSHIP_ON_HOLD) == 0);
  assert(create_gym_member("Name", "", "0171111111", "B1", "u2", "h1", plan, MEMBERSHIP_ON_HOLD) == 0);
  assert(create_gym_member("Name", "a@t.com", "", "B1", "u3", "h1", plan, MEMBERSHIP_ON_HOLD) == 0);
  assert(create_gym_member("Name", "a@t.com", "0171111111", "", "u4", "h1", plan, MEMBERSHIP_ON_HOLD) == 0);
  assert(create_gym_member("Name", "a@t.com", "0171111111", "B1", "", "h1", plan, MEMBERSHIP_ON_HOLD) == 0);
  assert(create_gym_member("Name", "a@t.com", "0171111111", "B1", "u5", "", plan, MEMBERSHIP_ON_HOLD) == 0);

  cleanup_user_files();
}

static void test_get_gym_member_by_username()
{
  cleanup_user_files();
  load_sysadmins();
  load_branch_staff();
  load_gym_members();

  subscription_plan_t plan;
  plan.payable_amount = 2000;
  plan.interval_days = 60;

  create_gym_member("Rina", "rina@t.com", "0191111111", "Mirpur", "rina", "hash", plan, MEMBERSHIP_ACTIVE);
  gym_member_t *found = get_gym_member_by_username("rina");
  assert(found != NULL);
  assert(strcmp(found->gym_branch, "Mirpur") == 0);
  assert(found->status == MEMBERSHIP_ACTIVE);

  assert(get_gym_member_by_username("nobody") == NULL);

  cleanup_user_files();
}

static void test_get_gym_member_by_id_not_found()
{
  cleanup_user_files();
  load_sysadmins();
  load_branch_staff();
  load_gym_members();

  assert(get_gym_member_by_id(999) == NULL);

  cleanup_user_files();
}

static void test_load_gym_members_roundtrip()
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
  gym_member_t *found = get_gym_member_by_username("tester_m");
  assert(found != NULL);
  assert(found->plan.payable_amount == 1500);
  assert(found->plan.interval_days == 30);
  assert(strcmp(found->password_hash, "hashval") == 0);

  cleanup_user_files();
}

// ---- username_exists tests ----

static void test_username_exists_returns_false_when_empty()
{
  cleanup_user_files();
  load_sysadmins();
  load_branch_staff();
  load_gym_members();

  assert(username_exists("anyone") == false);

  cleanup_user_files();
}

static void test_username_exists_finds_sysadmin()
{
  cleanup_user_files();
  load_sysadmins();
  load_branch_staff();
  load_gym_members();

  create_sysadmin("superadmin", "hash");
  assert(username_exists("superadmin") == true);

  cleanup_user_files();
}

static void test_username_exists_finds_branch_staff()
{
  cleanup_user_files();
  load_sysadmins();
  load_branch_staff();
  load_gym_members();

  create_branch_staff("Name", "e@t.com", "0171111111", "B1", "staff_user", "hash", TRAINER);
  assert(username_exists("staff_user") == true);

  cleanup_user_files();
}

static void test_username_exists_finds_gym_member()
{
  cleanup_user_files();
  load_sysadmins();
  load_branch_staff();
  load_gym_members();

  subscription_plan_t plan;
  plan.payable_amount = 1000;
  plan.interval_days = 30;

  create_gym_member("Name", "e@t.com", "0171111111", "B1", "member_user", "hash", plan, MEMBERSHIP_ON_HOLD);
  assert(username_exists("member_user") == true);

  cleanup_user_files();
}

static void test_username_exists_cross_table_uniqueness()
{
  cleanup_user_files();
  load_sysadmins();
  load_branch_staff();
  load_gym_members();

  create_sysadmin("shared_name", "hash1");

  subscription_plan_t plan;
  plan.payable_amount = 1000;
  plan.interval_days = 30;

  id_t staff_id = create_branch_staff("Name", "e@t.com", "0171111111", "B1", "shared_name", "hash2", TRAINER);
  assert(staff_id == 0);

  id_t member_id =
    create_gym_member("Name", "e@t.com", "0171111111", "B1", "shared_name", "hash3", plan, MEMBERSHIP_ON_HOLD);
  assert(member_id == 0);

  cleanup_user_files();
}

static void test_username_exists_null()
{
  cleanup_user_files();
  load_sysadmins();
  load_branch_staff();
  load_gym_members();

  assert(username_exists(NULL) == false);

  cleanup_user_files();
}

// ---- branch count tests ----

static void test_branch_manager_count_zero_when_empty()
{
  cleanup_user_files();
  load_sysadmins();
  load_branch_staff();
  load_gym_members();

  assert(branch_manager_count("Dhanmondi") == 0);

  cleanup_user_files();
}

static void test_branch_manager_count_one()
{
  cleanup_user_files();
  load_sysadmins();
  load_branch_staff();
  load_gym_members();

  create_branch_staff("M1", "m1@t.com", "0171111111", "Dhanmondi", "mgr1", "h1", BRANCH_MANAGER);
  assert(branch_manager_count("Dhanmondi") == 1);

  cleanup_user_files();
}

static void test_branch_manager_count_ignores_trainers()
{
  cleanup_user_files();
  load_sysadmins();
  load_branch_staff();
  load_gym_members();

  create_branch_staff("T1", "t1@t.com", "0171111111", "Dhanmondi", "tr1", "h1", TRAINER);
  create_branch_staff("T2", "t2@t.com", "0172222222", "Dhanmondi", "tr2", "h2", TRAINER);
  assert(branch_manager_count("Dhanmondi") == 0);

  cleanup_user_files();
}

static void test_branch_manager_count_ignores_other_branches()
{
  cleanup_user_files();
  load_sysadmins();
  load_branch_staff();
  load_gym_members();

  create_branch_staff("M1", "m1@t.com", "0171111111", "Banani", "mgr_b", "h1", BRANCH_MANAGER);
  assert(branch_manager_count("Dhanmondi") == 0);

  cleanup_user_files();
}

static void test_branch_trainer_count_zero_when_empty()
{
  cleanup_user_files();
  load_sysadmins();
  load_branch_staff();
  load_gym_members();

  assert(branch_trainer_count("Dhanmondi") == 0);

  cleanup_user_files();
}

static void test_branch_trainer_count_multiple()
{
  cleanup_user_files();
  load_sysadmins();
  load_branch_staff();
  load_gym_members();

  create_branch_staff("T1", "t1@t.com", "0171111111", "Dhanmondi", "tr1", "h1", TRAINER);
  create_branch_staff("T2", "t2@t.com", "0172222222", "Dhanmondi", "tr2", "h2", TRAINER);
  create_branch_staff("T3", "t3@t.com", "0173333333", "Dhanmondi", "tr3", "h3", TRAINER);
  assert(branch_trainer_count("Dhanmondi") == 3);

  cleanup_user_files();
}

static void test_branch_trainer_count_ignores_managers()
{
  cleanup_user_files();
  load_sysadmins();
  load_branch_staff();
  load_gym_members();

  create_branch_staff("M1", "m1@t.com", "0171111111", "Dhanmondi", "mgr1", "h1", BRANCH_MANAGER);
  assert(branch_trainer_count("Dhanmondi") == 0);

  cleanup_user_files();
}

static void test_branch_member_count_zero_when_empty()
{
  cleanup_user_files();
  load_sysadmins();
  load_branch_staff();
  load_gym_members();

  assert(branch_member_count("Dhanmondi") == 0);

  cleanup_user_files();
}

static void test_branch_member_count_multiple()
{
  cleanup_user_files();
  load_sysadmins();
  load_branch_staff();
  load_gym_members();

  subscription_plan_t plan;
  plan.payable_amount = 1000;
  plan.interval_days = 30;

  create_gym_member("A", "a@t.com", "0171111111", "Dhanmondi", "m1", "h1", plan, MEMBERSHIP_ON_HOLD);
  create_gym_member("B", "b@t.com", "0172222222", "Dhanmondi", "m2", "h2", plan, MEMBERSHIP_ACTIVE);
  assert(branch_member_count("Dhanmondi") == 2);

  cleanup_user_files();
}

static void test_branch_member_count_ignores_other_branches()
{
  cleanup_user_files();
  load_sysadmins();
  load_branch_staff();
  load_gym_members();

  subscription_plan_t plan;
  plan.payable_amount = 1000;
  plan.interval_days = 30;

  create_gym_member("A", "a@t.com", "0171111111", "Banani", "m1", "h1", plan, MEMBERSHIP_ON_HOLD);
  assert(branch_member_count("Dhanmondi") == 0);

  cleanup_user_files();
}

static void test_branch_counts_null_and_empty()
{
  cleanup_user_files();
  load_sysadmins();
  load_branch_staff();
  load_gym_members();

  assert(branch_manager_count(NULL) == 0);
  assert(branch_manager_count("") == 0);
  assert(branch_trainer_count(NULL) == 0);
  assert(branch_trainer_count("") == 0);
  assert(branch_member_count(NULL) == 0);
  assert(branch_member_count("") == 0);

  cleanup_user_files();
}

// ---- cross-table username uniqueness ----

static void test_staff_username_blocks_member_creation()
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

  cleanup_user_files();
}

static void test_member_username_blocks_staff_creation()
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

  cleanup_user_files();
}

/**
 * Runs every user module unit test, aborting on the first failure.
 */
void run_all_user_tests()
{
  /* sysadmin */
  test_create_sysadmin_and_get();
  test_create_sysadmin_rejects_second();
  test_create_sysadmin_rejects_empty_username();
  test_create_sysadmin_rejects_null_username();
  test_create_sysadmin_rejects_empty_password();
  test_get_sysadmin_by_username();
  test_get_sysadmin_by_username_null();
  test_get_sysadmin_by_id_not_found();
  test_load_sysadmins_roundtrip();
  /* branch staff */
  test_create_branch_staff_and_get();
  test_create_branch_staff_auto_increment_id();
  test_create_branch_staff_rejects_duplicate_username();
  test_create_branch_staff_rejects_empty_fields();
  test_get_branch_staff_by_username();
  test_get_branch_staff_by_id_not_found();
  test_load_branch_staff_roundtrip();
  /* gym member */
  test_create_gym_member_and_get();
  test_create_gym_member_auto_increment_id();
  test_create_gym_member_rejects_duplicate_username();
  test_create_gym_member_rejects_empty_fields();
  test_get_gym_member_by_username();
  test_get_gym_member_by_id_not_found();
  test_load_gym_members_roundtrip();
  /* username_exists */
  test_username_exists_returns_false_when_empty();
  test_username_exists_finds_sysadmin();
  test_username_exists_finds_branch_staff();
  test_username_exists_finds_gym_member();
  test_username_exists_cross_table_uniqueness();
  test_username_exists_null();
  /* branch counts */
  test_branch_manager_count_zero_when_empty();
  test_branch_manager_count_one();
  test_branch_manager_count_ignores_trainers();
  test_branch_manager_count_ignores_other_branches();
  test_branch_trainer_count_zero_when_empty();
  test_branch_trainer_count_multiple();
  test_branch_trainer_count_ignores_managers();
  test_branch_member_count_zero_when_empty();
  test_branch_member_count_multiple();
  test_branch_member_count_ignores_other_branches();
  test_branch_counts_null_and_empty();
  /* cross-table uniqueness */
  test_staff_username_blocks_member_creation();
  test_member_username_blocks_staff_creation();
}

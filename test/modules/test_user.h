#ifndef GYMTRAC_TEST_USER_H
#define GYMTRAC_TEST_USER_H

/** Removes all user data files; call once at startup from test_main. */
void cleanup_user_files();

/* sysadmin */
void test_create_sysadmin_and_get();
void test_create_sysadmin_rejects_second();
void test_create_sysadmin_rejects_empty_username();
void test_create_sysadmin_rejects_null_username();
void test_create_sysadmin_rejects_empty_password();
void test_get_sysadmin_by_username();
void test_get_sysadmin_by_username_null();
void test_get_sysadmin_by_id_not_found();
void test_load_sysadmins_roundtrip();

/* branch staff */
void test_create_branch_staff_and_get();
void test_create_branch_staff_auto_increment_id();
void test_create_branch_staff_rejects_duplicate_username();
void test_create_branch_staff_rejects_empty_fields();
void test_get_branch_staff_by_username();
void test_get_branch_staff_by_id_not_found();
void test_load_branch_staff_roundtrip();

/* gym member */
void test_create_gym_member_and_get();
void test_create_gym_member_auto_increment_id();
void test_create_gym_member_rejects_duplicate_username();
void test_create_gym_member_rejects_empty_fields();
void test_get_gym_member_by_username();
void test_get_gym_member_by_id_not_found();
void test_load_gym_members_roundtrip();

/* username_exists */
void test_username_exists_returns_false_when_empty();
void test_username_exists_finds_sysadmin();
void test_username_exists_finds_branch_staff();
void test_username_exists_finds_gym_member();
void test_username_exists_cross_table_uniqueness();
void test_username_exists_null();

/* branch counts */
void test_branch_manager_count_zero_when_empty();
void test_branch_manager_count_one();
void test_branch_manager_count_ignores_trainers();
void test_branch_manager_count_ignores_other_branches();
void test_branch_trainer_count_zero_when_empty();
void test_branch_trainer_count_multiple();
void test_branch_trainer_count_ignores_managers();
void test_branch_member_count_zero_when_empty();
void test_branch_member_count_multiple();
void test_branch_member_count_ignores_other_branches();
void test_branch_counts_null_and_empty();

/* cross-table uniqueness */
void test_staff_username_blocks_member_creation();
void test_member_username_blocks_staff_creation();

/* deletion */
void test_delete_branch_staff_removes_and_persists();
void test_delete_branch_staff_rejects_unknown_id();
void test_delete_gym_member_removes_and_persists();
void test_delete_gym_member_rejects_member_with_dues();

/* update */
void test_update_branch_staff_updates_fields_and_persists();
void test_update_branch_staff_rejects_unknown_id();
void test_update_branch_staff_rejects_empty_fields();
void test_update_gym_member_updates_fields_and_persists();
void test_update_gym_member_rejects_unknown_id();
void test_update_gym_member_rejects_empty_fields();
void test_update_gym_member_rejects_duplicate_username();
void test_update_gym_member_allows_same_username();

#endif

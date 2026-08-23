#ifndef GYMTRAC_TEST_BRANCH_H
#define GYMTRAC_TEST_BRANCH_H

/** Removes the branches file; call once at startup from test_main. */
void cleanup_branches_file();

void test_add_branch_and_exists();
void test_add_branch_rejects_duplicate();
void test_add_branch_rejects_empty();
void test_add_branch_rejects_null();
void test_find_branch_returns_negative_for_missing();
void test_find_branch_returns_negative_for_null();
void test_load_branches();
void test_load_branches_returns_zero_when_empty();
void test_add_branch_respects_max_count();
void test_branch_names_are_case_sensitive();
void test_get_branch_count();
void test_get_branch_name();
void test_ensure_branch_has_no_users();
void test_delete_branch_removes_and_persists();
void test_delete_branch_rejects_missing_null_and_empty();
void test_delete_branch_rejects_branch_with_users();
void test_update_branch_name_renames_and_persists();
void test_update_branch_name_cascades_to_user_records();
void test_update_branch_name_rejects_invalid_names();

#endif

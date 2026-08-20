#ifndef GYMTRAC_TEST_BRANCH_H
#define GYMTRAC_TEST_BRANCH_H

/** Removes the branches file; call once at startup from test_main. */
void cleanup_branches_file();

void test_add_branch_and_exists();
void test_add_branch_rejects_duplicate();
void test_add_branch_rejects_empty();
void test_add_branch_rejects_null();
void test_branch_exists_returns_false_for_missing();
void test_branch_exists_returns_false_for_null();
void test_load_branches();
void test_load_branches_returns_zero_when_empty();
void test_add_branch_respects_max_count();
void test_branch_names_are_case_sensitive();
void test_get_branch_count();
void test_get_branch_name();

#endif

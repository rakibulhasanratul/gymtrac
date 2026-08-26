#ifndef GYMTRAC_TEST_LOST_FOUND_H
#define GYMTRAC_TEST_LOST_FOUND_H

/**
 * Removes the lost and found data files from the test_data directory.
 * Called once at startup from test_main and between scenario resets.
 */
void cleanup_lost_found_files();

void test_report_item_persists_with_branch_snapshot_and_open_state();
void test_report_works_for_any_role();
void test_get_lost_and_found_scopes_by_branch_reporter_and_resolver();
void test_resolve_lost_item_by_manager_and_sysadmin();
void test_report_rejects_invalid_input();
void test_resolve_rejects_invalid_input();

#endif

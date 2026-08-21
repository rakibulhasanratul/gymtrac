#ifndef GYMTRAC_TEST_SESSION_H
#define GYMTRAC_TEST_SESSION_H

void test_session_init_is_inactive();
void test_session_login_sets_context();
void test_session_logout_clears_context();
void test_session_is_active_after_login();
void test_session_is_active_after_logout();
void test_session_is_sysadmin();
void test_session_is_branch_manager();
void test_session_is_trainer();
void test_session_is_member();
void test_session_belongs_to_branch_sysadmin_sees_all();
void test_session_belongs_to_branch_matching();
void test_session_belongs_to_branch_non_matching();
void test_session_belongs_to_branch_inactive_returns_false();
void test_session_belongs_to_branch_null_returns_false();
void test_session_get_current_returns_null_when_inactive();
void test_session_get_current_returns_record_when_active();

#endif

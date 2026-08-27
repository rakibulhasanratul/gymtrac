#include <assert.h>
#include <string.h>

#include "../../src/modules/session.h"
#include "../../src/types.h"
#include "test_session.h"

void test_session_init_is_inactive()
{
  initialize_session();
  assert(session_is_active() == false);
}

void test_session_login_sets_context()
{
  initialize_session();
  set_session_context(USER_ROLE_TRAINER, 5, "rahim", "Dhanmondi");

  assert(session_is_active() == true);
  assert(get_role_from_session() == USER_ROLE_TRAINER);
  assert(get_user_id_from_session() == 5);
  assert(strcmp(get_username_from_session(), "rahim") == 0);
  assert(strcmp(get_branch_name_from_session(), "Dhanmondi") == 0);
}

void test_session_logout_clears_context()
{
  initialize_session();
  set_session_context(USER_ROLE_MEMBER, 3, "nusrat", "Uttara");
  clear_session_context();

  assert(session_is_active() == false);
  assert(get_user_id_from_session() == 0);
  assert(get_role_from_session() == 0);
  assert(strcmp(get_username_from_session(), "") == 0);
  assert(strcmp(get_branch_name_from_session(), "") == 0);
}

void test_session_is_active_after_login()
{
  initialize_session();
  set_session_context(USER_ROLE_SYSADMIN, 1, "admin", "");
  assert(session_is_active() == true);
}

void test_session_is_active_after_logout()
{
  initialize_session();
  set_session_context(USER_ROLE_SYSADMIN, 1, "admin", "");
  clear_session_context();
  assert(session_is_active() == false);
}

void test_session_is_sysadmin()
{
  initialize_session();
  set_session_context(USER_ROLE_SYSADMIN, 1, "admin", "");
  assert(session_is_sysadmin() == true);
  assert(session_is_branch_manager() == false);
  assert(session_is_trainer() == false);
  assert(session_is_member() == false);
}

void test_session_is_branch_manager()
{
  initialize_session();
  set_session_context(USER_ROLE_BRANCH_MANAGER, 2, "mgr1", "Gulshan");
  assert(session_is_branch_manager() == true);
  assert(session_is_sysadmin() == false);
  assert(session_is_trainer() == false);
  assert(session_is_member() == false);
}

void test_session_is_trainer()
{
  initialize_session();
  set_session_context(USER_ROLE_TRAINER, 3, "tr1", "Banani");
  assert(session_is_trainer() == true);
  assert(session_is_sysadmin() == false);
  assert(session_is_branch_manager() == false);
  assert(session_is_member() == false);
}

void test_session_is_member()
{
  initialize_session();
  set_session_context(USER_ROLE_MEMBER, 4, "mem1", "Uttara");
  assert(session_is_member() == true);
  assert(session_is_sysadmin() == false);
  assert(session_is_branch_manager() == false);
  assert(session_is_trainer() == false);
}

void test_session_belongs_to_branch_sysadmin_sees_all()
{
  initialize_session();
  set_session_context(USER_ROLE_SYSADMIN, 1, "admin", "");
  assert(session_belongs_to_branch("Dhanmondi") == true);
  assert(session_belongs_to_branch("AnyBranch") == true);
}

void test_session_belongs_to_branch_matching()
{
  initialize_session();
  set_session_context(USER_ROLE_TRAINER, 3, "tr1", "Dhanmondi");
  assert(session_belongs_to_branch("Dhanmondi") == true);
}

void test_session_belongs_to_branch_non_matching()
{
  initialize_session();
  set_session_context(USER_ROLE_TRAINER, 3, "tr1", "Dhanmondi");
  assert(session_belongs_to_branch("Gulshan") == false);
}

void test_session_belongs_to_branch_inactive_returns_false()
{
  initialize_session();
  assert(session_belongs_to_branch("Dhanmondi") == false);
}

void test_session_belongs_to_branch_null_returns_false()
{
  initialize_session();
  set_session_context(USER_ROLE_TRAINER, 3, "tr1", "Dhanmondi");
  assert(session_belongs_to_branch(NULL) == false);
  assert(session_belongs_to_branch("") == false);
}

void test_session_getters_return_empty_when_inactive()
{
  initialize_session();
  assert(session_is_active() == false);
  assert(get_user_id_from_session() == 0);
  assert(get_role_from_session() == 0);
  assert(strcmp(get_username_from_session(), "") == 0);
  assert(strcmp(get_branch_name_from_session(), "") == 0);
}

void test_session_getters_return_values_when_active()
{
  initialize_session();
  set_session_context(USER_ROLE_MEMBER, 7, "fatema", "Mirpur");
  assert(session_is_active() == true);
  assert(get_user_id_from_session() == 7);
  assert(get_role_from_session() == USER_ROLE_MEMBER);
  assert(strcmp(get_username_from_session(), "fatema") == 0);
  assert(strcmp(get_branch_name_from_session(), "Mirpur") == 0);
}

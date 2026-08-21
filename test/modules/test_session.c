#include <assert.h>
#include <string.h>

#include "../../src/modules/session.h"
#include "../../src/types.h"
#include "test_session.h"

void test_session_init_is_inactive()
{
  session_init();
  assert(session_is_active() == false);
}

void test_session_login_sets_context()
{
  session_init();
  session_login(USER_ROLE_TRAINER, 5, "rahim", "Dhanmondi");

  const session_t *s = session_get_current();
  assert(s != NULL);
  assert(s->role == USER_ROLE_TRAINER);
  assert(s->user_id == 5);
  assert(strcmp(s->username, "rahim") == 0);
  assert(strcmp(s->branch_name, "Dhanmondi") == 0);
}

void test_session_logout_clears_context()
{
  session_init();
  session_login(USER_ROLE_MEMBER, 3, "nusrat", "Uttara");
  session_logout();

  assert(session_is_active() == false);
  assert(session_get_current() == NULL);
}

void test_session_is_active_after_login()
{
  session_init();
  session_login(USER_ROLE_SYSADMIN, 1, "admin", "");
  assert(session_is_active() == true);
}

void test_session_is_active_after_logout()
{
  session_init();
  session_login(USER_ROLE_SYSADMIN, 1, "admin", "");
  session_logout();
  assert(session_is_active() == false);
}

void test_session_is_sysadmin()
{
  session_init();
  session_login(USER_ROLE_SYSADMIN, 1, "admin", "");
  assert(session_is_sysadmin() == true);
  assert(session_is_branch_manager() == false);
  assert(session_is_trainer() == false);
  assert(session_is_member() == false);
}

void test_session_is_branch_manager()
{
  session_init();
  session_login(USER_ROLE_BRANCH_MANAGER, 2, "mgr1", "Gulshan");
  assert(session_is_branch_manager() == true);
  assert(session_is_sysadmin() == false);
  assert(session_is_trainer() == false);
  assert(session_is_member() == false);
}

void test_session_is_trainer()
{
  session_init();
  session_login(USER_ROLE_TRAINER, 3, "tr1", "Banani");
  assert(session_is_trainer() == true);
  assert(session_is_sysadmin() == false);
  assert(session_is_branch_manager() == false);
  assert(session_is_member() == false);
}

void test_session_is_member()
{
  session_init();
  session_login(USER_ROLE_MEMBER, 4, "mem1", "Uttara");
  assert(session_is_member() == true);
  assert(session_is_sysadmin() == false);
  assert(session_is_branch_manager() == false);
  assert(session_is_trainer() == false);
}

void test_session_belongs_to_branch_sysadmin_sees_all()
{
  session_init();
  session_login(USER_ROLE_SYSADMIN, 1, "admin", "");
  assert(session_belongs_to_branch("Dhanmondi") == true);
  assert(session_belongs_to_branch("AnyBranch") == true);
}

void test_session_belongs_to_branch_matching()
{
  session_init();
  session_login(USER_ROLE_TRAINER, 3, "tr1", "Dhanmondi");
  assert(session_belongs_to_branch("Dhanmondi") == true);
}

void test_session_belongs_to_branch_non_matching()
{
  session_init();
  session_login(USER_ROLE_TRAINER, 3, "tr1", "Dhanmondi");
  assert(session_belongs_to_branch("Gulshan") == false);
}

void test_session_belongs_to_branch_inactive_returns_false()
{
  session_init();
  assert(session_belongs_to_branch("Dhanmondi") == false);
}

void test_session_belongs_to_branch_null_returns_false()
{
  session_init();
  session_login(USER_ROLE_TRAINER, 3, "tr1", "Dhanmondi");
  assert(session_belongs_to_branch(NULL) == false);
  assert(session_belongs_to_branch("") == false);
}

void test_session_get_current_returns_null_when_inactive()
{
  session_init();
  assert(session_get_current() == NULL);
}

void test_session_get_current_returns_record_when_active()
{
  session_init();
  session_login(USER_ROLE_MEMBER, 7, "fatema", "Mirpur");
  const session_t *s = session_get_current();
  assert(s != NULL);
  assert(s->user_id == 7);
  assert(strcmp(s->username, "fatema") == 0);
}

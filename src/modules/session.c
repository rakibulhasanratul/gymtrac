#include <stdbool.h>
#include <string.h>

#include "../types.h"
#include "../utils/string_util.h"
#include "session.h"

static session_t _session;

void initialize_session()
{
  _session.role = 0;
  _session.user_id = 0;
  _session.username[0] = '\0';
  _session.branch_name[0] = '\0';
}

void set_session_context(user_role_t role, id_t user_id, const char username[], const char branch_name[])
{
  _session.role = role;
  _session.user_id = user_id;

  if (username != NULL)
    strcpy(_session.username, username);
  else
    _session.username[0] = '\0';

  if (branch_name != NULL)
    strcpy(_session.branch_name, branch_name);
  else
    _session.branch_name[0] = '\0';
}

void clear_session_context()
{
  initialize_session();
}

bool session_is_active()
{
  return _session.user_id != 0;
}

bool session_is_sysadmin()
{
  return session_is_active() && _session.role == USER_ROLE_SYSADMIN;
}

bool session_is_branch_manager()
{
  return session_is_active() && _session.role == USER_ROLE_BRANCH_MANAGER;
}

bool session_is_trainer()
{
  return session_is_active() && _session.role == USER_ROLE_TRAINER;
}

bool session_is_member()
{
  return session_is_active() && _session.role == USER_ROLE_MEMBER;
}

bool session_belongs_to_branch(const char branch_name[])
{
  if (!session_is_active()) return false;

  if (is_blank_string(branch_name)) return false;

  if (_session.role == USER_ROLE_SYSADMIN) return true;

  return strcmp(_session.branch_name, branch_name) == 0;
}

user_role_t get_role_from_session()
{
  return _session.role;
}

id_t get_user_id_from_session()
{
  return _session.user_id;
}

const char *get_username_from_session()
{
  return _session.username;
}

const char *get_branch_name_from_session()
{
  return _session.branch_name;
}

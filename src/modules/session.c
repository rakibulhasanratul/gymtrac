#include <stdbool.h>
#include <string.h>

#include "../types.h"
#include "session.h"

static session_t current_session;

void session_init()
{
  memset(&current_session, 0, sizeof(session_t));
}

void set_session_context(user_role_t role, id_t user_id, const char username[], const char branch_name[])
{
  current_session.role = role;
  current_session.user_id = user_id;

  if (username != NULL)
    strcpy(current_session.username, username);
  else
    current_session.username[0] = '\0';

  if (branch_name != NULL)
    strcpy(current_session.branch_name, branch_name);
  else
    current_session.branch_name[0] = '\0';
}

void clear_session_context()
{
  memset(&current_session, 0, sizeof(session_t));
}

bool session_is_active()
{
  return current_session.user_id != 0;
}

bool session_is_sysadmin()
{
  return session_is_active() && current_session.role == USER_ROLE_SYSADMIN;
}

bool session_is_branch_manager()
{
  return session_is_active() && current_session.role == USER_ROLE_BRANCH_MANAGER;
}

bool session_is_trainer()
{
  return session_is_active() && current_session.role == USER_ROLE_TRAINER;
}

bool session_is_member()
{
  return session_is_active() && current_session.role == USER_ROLE_MEMBER;
}

bool session_belongs_to_branch(const char branch_name[])
{
  if (!session_is_active())
    return false;

  if (branch_name == NULL || strlen(branch_name) == 0)
    return false;

  if (current_session.role == USER_ROLE_SYSADMIN)
    return true;

  return strcmp(current_session.branch_name, branch_name) == 0;
}

const session_t *session_get_current()
{
  if (!session_is_active())
    return NULL;

  return &current_session;
}

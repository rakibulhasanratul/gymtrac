#ifndef GYMTRAC_SESSION_H
#define GYMTRAC_SESSION_H

#include <stdbool.h>

#include "../types.h"

/**
 * Initializes the session as logged out (all fields zeroed).
 *
 * Should be called once at startup.
 */
void initialize_session();

/**
 * Populates the session with the logged-in user's context.
 *
 * @param role the user's top-level role
 * @param user_id the user's record id
 * @param username the user's login name
 * @param branch_name the user's branch (empty string for sysadmin)
 */
void set_session_context(user_role_t role, id_t user_id, const char username[], const char branch_name[]);

/**
 * Clears the session, marking the user as logged out.
 */
void clear_session_context();

/**
 * Returns whether a user is currently logged in.
 *
 * @return true if the session is active, false otherwise
 */
bool session_is_active();

/**
 * Returns whether the logged-in user is a system administrator.
 */
bool session_is_sysadmin();

/**
 * Returns whether the logged-in user is a branch manager.
 */
bool session_is_branch_manager();

/**
 * Returns whether the logged-in user is a branch trainer.
 */
bool session_is_trainer();

/**
 * Returns whether the logged-in user is a gym member.
 */
bool session_is_member();

/**
 * Returns whether the logged-in user belongs to the given branch.
 *
 * Sysadmins belong to every branch (always returns true).
 * Returns false if the session is inactive or branch_name is empty/null.
 *
 * @param branch_name the branch to check
 * @return true if the user's branch matches or the user is a sysadmin
 */
bool session_belongs_to_branch(const char branch_name[]);

/**
 * Returns the logged-in user's role.
 *
 * @return the role stored in the session, or 0 if no session is active
 */
user_role_t get_role_from_session();

/**
 * Returns the logged-in user's id.
 *
 * @return the user id stored in the session, or 0 if no session is active
 */
id_t get_user_id_from_session();

/**
 * Returns the logged-in user's username.
 *
 * @return pointer to the internal username buffer, empty string if inactive
 */
const char *get_username_from_session();

/**
 * Returns the logged-in user's branch name.
 *
 * @return pointer to the internal branch_name buffer, empty string if inactive
 */
const char *get_branch_name_from_session();

#endif

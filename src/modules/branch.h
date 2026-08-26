#ifndef GYMTRAC_BRANCH_H
#define GYMTRAC_BRANCH_H

#include <stdbool.h>

#include "../settings.h"

/**
 * Loads branch names from the persisted file into the in-memory array.
 *
 * Should be called once at startup before any branch operations.
 *
 * @return the number of branches loaded
 */
int load_branches();

/**
 * Finds a branch by its exact name in the in-memory array.
 *
 * The lookup is a case-sensitive exact match.
 *
 * @param branch_name the name to look up
 * @return the zero-based index of the matching branch, or -1 if not found
 */
int find_branch(const char branch_name[]);

/**
 * Appends a new branch name to the persisted file and in-memory array.
 *
 * The branch is rejected if it already exists, the name is empty, or
 * the maximum branch count is reached.
 *
 * @param branch_name the name to add
 * @return true if the branch was added, false otherwise
 */
bool add_branch(const char branch_name[]);

/**
 * Policy guard ensuring no staff or gym member is assigned to a branch.
 *
 * A branch with any manager, trainer, or member stays undeletable, so
 * records never reference a removed branch.
 *
 * @param branch_name the branch to check
 * @return true when the branch name is valid and has no users, false otherwise
 */
bool ensure_branch_has_no_users(const char branch_name[]);

/**
 * Removes a branch from the persisted file and in-memory array.
 *
 * Rejected if the name is empty, the branch does not exist, or any
 * staff or member is still assigned to it (see ensure_branch_has_no_users).
 *
 * @param branch_name the name to delete
 * @return true if the branch was deleted, false otherwise
 */
bool delete_branch(const char branch_name[]);

/**
 * Renames a branch across the in-memory array, the persisted branches file,
 * and every staff or member record assigned to it.
 *
 * Rejected when either name is empty, the old name does not exist, or the
 * new name is already taken. Staff and member records are cascaded first,
 * then the branch list itself is updated at the matched index and rewritten.
 *
 * @param old_branch_name the branch to rename
 * @param new_branch_name the replacement branch name
 * @return true if the branch was renamed everywhere, false otherwise
 */
bool update_branch_name(const char old_branch_name[], const char new_branch_name[]);

/**
 * Returns the number of branches currently loaded in memory.
 *
 * @return the branch count
 */
int get_branch_count();

/**
 * Returns the branch name at the given index.
 *
 * @param index the zero-based index
 * @return the branch name, or NULL if index is out of range
 */
const char *get_branch_name(int index);

#endif

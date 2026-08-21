#ifndef GYMTRAC_USER_H
#define GYMTRAC_USER_H

#include <stdbool.h>

#include "../types.h"

/**
 * Loads sysadmin records from the persisted file into the in-memory array.
 *
 * Should be called once at startup. Resets the internal count before loading.
 *
 * @return the number of sysadmins loaded
 */
int load_sysadmins();

/**
 * Loads branch staff records from the persisted file into the in-memory array.
 *
 * Should be called once at startup. Resets the internal count before loading.
 *
 * @return the number of branch staff loaded
 */
int load_branch_staff();

/**
 * Loads gym member records from the persisted file into the in-memory array.
 *
 * Should be called once at startup. Resets the internal count before loading.
 *
 * @return the number of gym members loaded
 */
int load_gym_members();

/**
 * Creates a new sysadmin with an auto-incremented id and persists the record.
 *
 * Rejected if a sysadmin already exists, the username is empty, or the
 * username already exists in any user table.
 *
 * @param username the unique login name
 * @param password_hash the pre-hashed password string
 * @return the new sysadmin's id, or 0 on failure
 */
id_t create_sysadmin(const char username[], const char password_hash[]);

/**
 * Creates a new branch staff member with an auto-incremented id and persists
 * the record.
 *
 * Rejected if the username already exists in any user table, required fields
 * are empty, or the in-memory capacity is reached.
 *
 * @param full_name the staff member's full name
 * @param email the staff member's email
 * @param phone_number the staff member's phone number
 * @param gym_branch the branch the staff member belongs to
 * @param username the unique login name
 * @param password_hash the pre-hashed password string
 * @param role TRAINER or BRANCH_MANAGER
 * @return the new staff member's id, or 0 on failure
 */
id_t create_branch_staff(const char full_name[], const char email[], const char phone_number[], const char gym_branch[],
                         const char username[], const char password_hash[], staff_role_t role);

/**
 * Creates a new gym member with an auto-incremented id and persists the record.
 *
 * Rejected if the username already exists in any user table, required fields
 * are empty, or the in-memory capacity is reached.
 *
 * @param full_name the member's full name
 * @param email the member's email
 * @param phone_number the member's phone number
 * @param gym_branch the branch the member belongs to
 * @param username the unique login name
 * @param password_hash the pre-hashed password string
 * @param plan the subscription plan
 * @param status the membership status
 * @return the new member's id, or 0 on failure
 */
id_t create_gym_member(const char full_name[], const char email[], const char phone_number[], const char gym_branch[],
                       const char username[], const char password_hash[], subscription_plan_t plan_payload,
                       membership_status_t status);

/**
 * Policy guard ensuring a gym member carries no outstanding dues.
 *
 * A member with unpaid dues can never be deleted so recorded debt is
 * never silently erased.
 *
 * @param member_payload the member record to check
 * @return true when the record is valid and due_amount is zero, false otherwise
 */
bool ensure_member_has_no_dues(const gym_member_t *member_payload);

/**
 * Removes a branch staff member from the persisted file and in-memory array.
 *
 * Sysadmins have no delete path because removing the sole admin would
 * leave nobody able to log in.
 *
 * @param id the staff member's id
 * @return true if the staff member was deleted, false if not found
 */
bool delete_branch_staff(id_t id);

/**
 * Removes a gym member from the persisted file and in-memory array.
 *
 * Rejected when the member still owes dues (see ensure_member_has_no_dues).
 *
 * @param id the member's id
 * @return true if the member was deleted, false if not found or indebted
 */
bool delete_gym_member(id_t id);

/**
 * Checks whether a username exists in any of the three user tables.
 *
 * Usernames are globally unique across sysadmins, branch staff, and gym members.
 *
 * @param username the login name to check
 * @return true if the username is taken, false if available
 */
bool username_exists(const char username[]);

/**
 * Counts branch managers in the given branch.
 *
 * @param branch_name the branch to count managers for
 * @return the number of branch managers in that branch
 */
int branch_manager_count(const char branch_name[]);

/**
 * Counts branch trainers in the given branch.
 *
 * @param branch_name the branch to count trainers for
 * @return the number of branch trainers in that branch
 */
int branch_trainer_count(const char branch_name[]);

/**
 * Counts gym members in the given branch.
 *
 * @param branch_name the branch to count members for
 * @return the number of gym members in that branch
 */
int branch_member_count(const char branch_name[]);

/**
 * Finds a sysadmin by id.
 *
 * @param id the sysadmin's id
 * @return pointer to the sysadmin record, or NULL if not found
 */
sysadmin_t *get_sysadmin_by_id(id_t id);

/**
 * Finds a sysadmin by username.
 *
 * @param username the login name to look up
 * @return pointer to the sysadmin record, or NULL if not found
 */
sysadmin_t *get_sysadmin_by_username(const char username[]);

/**
 * Finds a branch staff member by id.
 *
 * @param id the staff member's id
 * @return pointer to the staff record, or NULL if not found
 */
branch_staff_t *get_branch_staff_by_id(id_t id);

/**
 * Finds a branch staff member by username.
 *
 * @param username the login name to look up
 * @return pointer to the staff record, or NULL if not found
 */
branch_staff_t *get_branch_staff_by_username(const char username[]);

/**
 * Finds a gym member by id.
 *
 * @param id the member's id
 * @return pointer to the member record, or NULL if not found
 */
gym_member_t *get_gym_member_by_id(id_t id);

/**
 * Finds a gym member by username.
 *
 * @param username the login name to look up
 * @return pointer to the member record, or NULL if not found
 */
gym_member_t *get_gym_member_by_username(const char username[]);

#endif

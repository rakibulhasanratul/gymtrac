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
id_t create_branch_staff(
  const char full_name[],
  const char email[],
  const char phone_number[],
  const char gym_branch[],
  const char username[],
  const char password_hash[],
  staff_role_t role
);

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
id_t create_gym_member(
  const char full_name[],
  const char email[],
  const char phone_number[],
  const char gym_branch[],
  const char username[],
  const char password_hash[],
  subscription_plan_t plan_payload,
  membership_status_t status
);

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
 * Rejected when the member still owes dues, so recorded debt is never
 * silently erased.
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
 * Finds a sysadmin by id and copies the record into destination.
 *
 * Callers work on their own snapshot so internal records can never be
 * modified through the getter.
 *
 * @param id the sysadmin's id
 * @param destination receives a copy of the record when found;
 *                    must point to a valid sysadmin_t
 * @return true when found and copied, false when not found or destination is NULL
 */
bool get_sysadmin_by_id(id_t id, sysadmin_t *destination);

/**
 * Finds a sysadmin by username and copies the record into destination.
 *
 * Callers work on their own snapshot so internal records can never be
 * modified through the getter.
 *
 * @param username the login name to look up
 * @param destination receives a copy of the record when found;
 *                    must point to a valid sysadmin_t
 * @return true when found and copied, false when not found or arguments are invalid
 */
bool get_sysadmin_by_username(const char username[], sysadmin_t *destination);

/**
 * Finds a branch staff member by id and copies the record into destination.
 *
 * Callers work on their own snapshot so internal records can never be
 * modified through the getter.
 *
 * @param id the staff member's id
 * @param destination receives a copy of the record when found;
 *                    must point to a valid branch_staff_t
 * @return true when found and copied, false when not found or destination is NULL
 */
bool get_branch_staff_by_id(id_t id, branch_staff_t *destination);

/**
 * Finds a branch staff member by username and copies the record into
 * destination.
 *
 * Callers work on their own snapshot so internal records can never be
 * modified through the getter.
 *
 * @param username the login name to look up
 * @param destination receives a copy of the record when found;
 *                    must point to a valid branch_staff_t
 * @return true when found and copied, false when not found or arguments are invalid
 */
bool get_branch_staff_by_username(const char username[], branch_staff_t *destination);

/**
 * Finds a gym member by id and copies the record into destination.
 *
 * Callers work on their own snapshot so internal records can never be
 * modified through the getter.
 *
 * @param id the member's id
 * @param destination receives a copy of the record when found;
 *                    must point to a valid gym_member_t
 * @return true when found and copied, false when not found or destination is NULL
 */
bool get_gym_member_by_id(id_t id, gym_member_t *destination);

/**
 * Finds a gym member by username and copies the record into destination.
 *
 * Callers work on their own snapshot so internal records can never be
 * modified through the getter.
 *
 * @param username the login name to look up
 * @param destination receives a copy of the record when found;
 *                    must point to a valid gym_member_t
 * @return true when found and copied, false when not found or arguments are invalid
 */
bool get_gym_member_by_username(const char username[], gym_member_t *destination);

/**
 * Updates a branch staff member's profile fields and persists the change.
 *
 * Only full_name, email, and phone_number can be updated. The record is
 * looked up by id; the file is rewritten with the updated values.
 *
 * @param id the staff member's id
 * @param full_name the new full name
 * @param email the new email
 * @param phone_number the new phone number
 * @return true if the record was found and updated, false otherwise
 */
bool update_branch_staff(id_t id, const char full_name[], const char email[], const char phone_number[]);

/**
 * Updates a gym member's profile fields and persists the change.
 *
 * Full_name, email, phone_number, gym_branch, and username can be updated.
 * The record is looked up by id; the file is rewritten with the updated values.
 *
 * @param id the member's id
 * @param full_name the new full name
 * @param email the new email
 * @param phone_number the new phone number
 * @param gym_branch the new branch name
 * @param username the new username (must be globally unique)
 * @return true if the record was found and updated, false otherwise
 */
bool update_gym_member(
  id_t id,
  const char full_name[],
  const char email[],
  const char phone_number[],
  const char gym_branch[],
  const char username[]
);

/**
 * Updates a gym member's membership status and persists the change.
 *
 * The record is looked up by id; the file is rewritten with the updated
 * value. For status-only transitions such as suspension and unsuspension.
 *
 * @param id the member's id
 * @param status the membership status to store on the record
 * @return true if the record was found and updated, false otherwise
 */
bool update_gym_member_status(id_t id, membership_status_t status);

/**
 * Updates a gym member's lifecycle fields and persists the change.
 *
 * Covers the plan, billing date, dues, and membership status in one write,
 * as driven by the approval flow. The record is looked up by id; the file
 * is rewritten with the updated values.
 *
 * @param id the member's id
 * @param plan_payload the subscription plan to store on the record
 * @param last_payment_date the billing cycle start date
 * @param due_amount the outstanding amount in whole Taka
 * @param status the membership status to store on the record
 * @return true if the record was found and updated, false otherwise
 */
bool update_gym_member_lifecycle(
  id_t id,
  subscription_plan_t plan_payload,
  time_t last_payment_date,
  unsigned int due_amount,
  membership_status_t status
);

/**
 * Moves every staff and member record from the old branch name to the new
 * one, in memory and on disk.
 *
 * Used by the branch rename flow so records never reference a branch name
 * that no longer exists. Sysadmins carry no branch and are untouched.
 *
 * @param old_branch_name the branch name to replace
 * @param new_branch_name the replacement branch name
 * @return true when every matching record was updated and persisted,
 *         false otherwise
 */
bool rename_branch_for_all_users(const char old_branch_name[], const char new_branch_name[]);

/**
 * Collects the ids of every loaded member carrying the given status.
 *
 * Copies at most destination_capacity ids; members beyond that are skipped.
 *
 * @param status the membership status to filter by
 * @param destination_ids receives the matching member ids
 * @param destination_capacity the number of slots available in destination_ids
 * @return the number of ids copied
 */
int get_gym_member_ids_by_status(membership_status_t status, id_t ids_destination[], int destination_capacity);

#endif

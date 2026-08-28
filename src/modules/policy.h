#ifndef GYMTRAC_POLICY_H
#define GYMTRAC_POLICY_H

#include <stdbool.h>

#include "../types.h"

/**
 * Checks whether the current session is allowed to approve a gym member.
 *
 * Sysadmins may approve any member. Branch managers may only approve members
 * assigned to their own branch.
 *
 * @param gym_member_id the member to approve
 * @return true when the operation is permitted, false otherwise
 */
bool ensure_membership_approval_is_allowed(id_t gym_member_id);

/**
 * Checks whether the current session is allowed to suspend a gym member.
 *
 * Sysadmins may suspend any member. Branch managers may only suspend members
 * assigned to their own branch.
 *
 * @param gym_member_id the member to suspend
 * @return true when the operation is permitted, false otherwise
 */
bool ensure_membership_suspension_is_allowed(id_t gym_member_id);

/**
 * Checks whether the current session is allowed to unsuspend a gym member.
 *
 * Sysadmins may unsuspend any member. Branch managers may only unsuspend
 * members assigned to their own branch.
 *
 * @param gym_member_id the member to unsuspend
 * @return true when the operation is permitted, false otherwise
 */
bool ensure_membership_unsuspension_is_allowed(id_t gym_member_id);

/**
 * Checks whether the current session is allowed to request a status change
 * for a gym member.
 *
 * Sysadmins may request status changes for any member. Branch trainers may
 * only request status changes for members assigned to their own branch.
 *
 * @param gym_member_id the member whose status change is being requested
 * @return true when the operation is permitted, false otherwise
 */
bool ensure_status_change_request_is_allowed(id_t gym_member_id);

/**
 * Checks whether the current session is allowed to request a plan change
 * for a gym member.
 *
 * Sysadmins may request plan changes for any member. Members may only
 * request plan changes for themselves.
 *
 * @param gym_member_id the member whose plan change is being requested
 * @return true when the operation is permitted, false otherwise
 */
bool ensure_plan_change_request_is_allowed(id_t gym_member_id);

/**
 * Checks whether the current session is allowed to request a profile edit
 * for a gym member.
 *
 * Sysadmins may request profile edits for any member. Members may only
 * request profile edits for themselves.
 *
 * @param gym_member_id the member whose profile edit is being requested
 * @return true when the operation is permitted, false otherwise
 */
bool ensure_profile_edit_request_is_allowed(id_t gym_member_id);

/**
 * Checks whether the current session is allowed to record a digital payment
 * for a gym member.
 *
 * Sysadmins may record digital payments for any member. Members may only
 * record digital payments for themselves.
 *
 * @param gym_member_id the member making the digital payment
 * @return true when the operation is permitted, false otherwise
 */
bool ensure_digital_payment_is_allowed(id_t gym_member_id);

/**
 * Checks whether the current session is allowed to record a cash payment
 * for a gym member.
 *
 * Sysadmins may record cash payments for any member. Branch trainers may
 * only record cash payments for members assigned to their own branch.
 *
 * @param gym_member_id the member receiving the cash payment
 * @return true when the operation is permitted, false otherwise
 */
bool ensure_cash_payment_recording_is_allowed(id_t gym_member_id);

/**
 * Checks whether the current session is allowed to view a gym member's
 * payment history.
 *
 * Sysadmins may view any member's payments. Members may only view their
 * own payments. Branch staff (managers and trainers) may view payments
 * for members in their own branch.
 *
 * @param gym_member_id the member whose payment history is being viewed
 * @return true when the operation is permitted, false otherwise
 */
bool ensure_payment_view_allowed(id_t gym_member_id);

/**
 * Checks whether the current session is allowed to view a gym member's
 * profile.
 *
 * Sysadmins may view any member's profile. Members may only view their
 * own profile. Branch staff (managers and trainers) may view profiles
 * for members in their own branch.
 *
 * @param gym_member_id the member whose profile is being viewed
 * @return true when the operation is permitted, false otherwise
 */
bool ensure_member_profile_view_allowed(id_t gym_member_id);

/**
 * Checks whether the current session is allowed to resolve a lost and
 * found report.
 *
 * Sysadmins may resolve any report. Branch managers may only resolve
 * reports in their own branch.
 *
 * @param item_payload the report whose branch gates the check
 * @return true when the operation is permitted, false otherwise
 */
bool ensure_lost_found_resolution_is_allowed(const lost_and_found_record_t item_payload);

/**
 * Checks whether the current session is allowed to delete a branch.
 *
 * Only the system administrator may delete branches. All other roles
 * are denied.
 *
 * @param branch_name the branch to delete
 * @return true when the operation is permitted, false otherwise
 */
bool ensure_branch_deletion_is_allowed(const char branch_name[]);

/**
 * Checks whether the current session is allowed to delete a gym member.
 *
 * Sysadmins may delete any member. Branch managers may only delete members
 * in their own branch, and only if the member has no outstanding dues.
 *
 * @param gym_member_id the member to delete
 * @return true when the operation is permitted, false otherwise
 */
bool ensure_member_deletion_is_allowed(id_t gym_member_id);

/**
 * Checks whether the current session is allowed to delete a branch staff
 * member.
 *
 * Sysadmins may delete any staff member. Branch managers may only delete
 * trainers (not other managers) in their own branch.
 *
 * @param staff_id the staff member to delete
 * @return true when the operation is permitted, false otherwise
 */
bool ensure_staff_deletion_is_allowed(id_t staff_id);

#endif

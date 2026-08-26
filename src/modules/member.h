#ifndef GYMTRAC_MEMBER_H
#define GYMTRAC_MEMBER_H

#include <stdbool.h>

#include "../types.h"

/**
 * Loads suspension records from the persisted file into the in-memory array.
 *
 * Should be called once at startup. Resets the internal count before loading.
 *
 * @return the number of suspension records loaded
 */
int load_suspensions();

/**
 * Approves an on-hold gym member, activating the membership.
 *
 * Assigns the default plan, starts billing today, and sets due_amount to the
 * plan's payable amount. Rejected when the member does not exist or is not on
 * hold; only active members can later be suspended.
 *
 * @param member_id the member to approve
 * @return true when the member was approved and persisted, false otherwise
 */
bool approve_gym_member(id_t member_id);

/**
 * Suspends an active gym member with a mandatory reason.
 *
 * Writes a dated suspension record whose unsuspension_date stays open until
 * unsuspend_gym_member closes it. Rejected when the member does not exist, is
 * not active, or the reason is empty.
 *
 * @param member_id the member to suspend
 * @param reason why the membership is being suspended; cannot be empty
 * @return true when the member was suspended and the record persisted,
 *         false otherwise
 */
bool suspend_gym_member(id_t member_id, const char reason[]);

/**
 * Reactivates a suspended gym member by closing their open suspension record.
 *
 * Stamps the open suspension record with today's date as the unsuspension
 * date. Rejected while the member still owes dues, when the member is not
 * suspended, or when no open suspension record exists.
 *
 * @param member_id the member to unsuspend
 * @return true when the member was reactivated and both records persisted,
 *         false otherwise
 */
bool unsuspend_gym_member(id_t member_id);

/**
 * Suspends every active member overdue past the dues grace period.
 *
 * A member is overdue when today is MAX_UNPAID_DAYS or more past their due
 * date (last_payment_date + plan.interval_days). Each swept member receives
 * a suspension record carrying AUTO_SUSPENSION_REASON dated today.
 *
 * @return the number of members auto-suspended by this sweep
 */
int auto_suspend_overdue_members();

/**
 * Copies every suspension recorded for a member into destination_records.
 *
 * Records come back in the order they were created, oldest first. At most
 * destination_capacity records are copied.
 *
 * @param gym_member_id the member whose suspension history to fetch
 * @param destination_records receives one suspension_record_t per match
 * @param destination_capacity the number of slots available in
 *                             destination_records
 * @return the number of records copied, 0 when the member has none
 */
int get_suspensions_for_member(id_t gym_member_id, suspension_record_t destination_records[], int destination_capacity);

#endif

#ifndef GYMTRAC_PAYMENT_H
#define GYMTRAC_PAYMENT_H

#include <stdbool.h>

#include "../types.h"

/**
 * Loads payments and their member join records from the persisted files into
 * the in-memory arrays.
 *
 * Should be called once at startup. Resets both internal counts before loading.
 *
 * @return the number of payments loaded
 */
int load_payments();

/**
 * Records a digital payment reported by a gym member.
 *
 * The payment is stored with the gateway-reported status carried by the
 * request, so pending, failed, and invalid attempts stay in the history too.
 * Only a PAYMENT_COMPLETED payment settles the account: it reduces the
 * member's due_amount (clamped at zero) and moves last_payment_date forward
 * to the transaction time. Rejected for unknown amounts, missing or empty
 * transaction references, unknown gateway statuses, completed payments
 * without a transaction time, and members that cannot receive payments.
 *
 * @param request_payload the digital payment details reported by the member
 * @return true when the payment was recorded, false otherwise
 */
bool record_digital_payment(const digital_payment_request_t request_payload);

/**
 * Records a cash payment handed to a branch trainer.
 *
 * Cash is trusted on handover, so the payment is stamped PAYMENT_COMPLETED
 * dated now and settles the member's account immediately: due_amount drops
 * by the paid amount (clamped at zero) and last_payment_date moves to now.
 * Rejected for zero amounts and members that cannot receive payments.
 *
 * @param gym_member_id the member whose account the cash pays into
 * @param amount the handed-over amount in whole Taka
 * @return true when the payment was recorded, false otherwise
 */
bool record_cash_payment(id_t gym_member_id, unsigned int amount);

/**
 * Copies every payment recorded for a member into destination_payments.
 *
 * Payments come back in the order they were recorded, oldest first. At most
 * destination_capacity payments are copied.
 *
 * @param gym_member_id the member whose payment history to fetch
 * @param destination_payments receives one payment_t per match
 * @param destination_capacity the number of slots available in
 *                             destination_payments
 * @return the number of payments copied, 0 when the member has none
 */
int get_payments_for_member(id_t gym_member_id, payment_t destination_payments[], int destination_capacity);

#endif

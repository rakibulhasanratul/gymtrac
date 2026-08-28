#ifndef GYMTRAC_LOST_FOUND_H
#define GYMTRAC_LOST_FOUND_H

#include <stdbool.h>

#include "../types.h"

/**
 * Loads lost and found records from the persisted file into the in-memory
 * array.
 *
 * Should be called once at startup. Resets the internal count before loading.
 *
 * @return the number of records loaded
 */
int load_lost_and_found_records();

/**
 * Records a lost or found item report submitted by any user.
 *
 * The reporter must be an existing user (member, staff, or system
 * administrator). The branch is snapshotted at report time; for staff and
 * members it matches their assigned branch, while the system administrator
 * may report against any branch. The report stays open until
 * resolve_lost_item stamps it with a resolver username.
 * Rejected for empty or oversized descriptions, empty branch names, and
 * usernames that belong to nobody.
 *
 * @param reporter_username the reporting user's username
 * @param gym_branch the branch where the item was found or lost
 * @param description what was lost or found; cannot be empty
 * @return true when the report was recorded, false otherwise
 */
bool report_lost_item(const char reporter_username[], const char gym_branch[], const char description[]);

/**
 * Marks an open lost and found record as resolved.
 *
 * Only branch managers and the system administrator may resolve reports.
 * Stamps resolver_username with the resolver's username, which doubles as the
 * resolved flag. Rejected when the record does not exist, is already resolved,
 * or the username belongs to a user who is not a manager or system
 * administrator.
 *
 * @param record_id the report to resolve
 * @param resolver_username the username of the resolving manager or
 *                          system administrator
 * @return true when the record was resolved and persisted, false otherwise
 */
bool resolve_lost_item(id_t record_id, const char resolver_username[]);

/**
 * Copies every report belonging to the given branch into destination.
 *
 * A report belongs to the branch stored on the record at report time. Records
 * come back in the order they were reported, oldest first, open and resolved
 * alike. At most destination_capacity records are copied.
 *
 * @param branch_name the branch whose reports to fetch
 * @param destination receives one lost_and_found_record_t per match
 * @param destination_capacity the number of slots available in destination
 * @return the number of records copied, 0 when the branch has none
 */
int get_lost_and_found_for_branch(
  const char branch_name[], lost_and_found_record_t *destination, int destination_capacity
);

/**
 * Copies every report submitted by the given user into destination.
 *
 * Records come back in the order they were reported, oldest first. At most
 * destination_capacity records are copied.
 *
 * @param reporter_username the reporting user's username
 * @param destination receives one lost_and_found_record_t per match
 * @param destination_capacity the number of slots available in destination
 * @return the number of records copied, 0 when the user reported nothing
 */
int get_lost_and_found_for_reporter(
  const char reporter_username[], lost_and_found_record_t *destination, int destination_capacity
);

/**
 * Copies every record resolved by the given user into destination.
 *
 * Records come back in the order they were reported, oldest first. At most
 * destination_capacity records are copied.
 *
 * @param resolver_username the resolver's username
 * @param destination receives one lost_and_found_record_t per match
 * @param destination_capacity the number of slots available in destination
 * @return the number of records copied, 0 when the user resolved nothing
 */
int get_lost_and_found_for_resolver(
  const char resolver_username[], lost_and_found_record_t *destination, int destination_capacity
);

/**
 * Finds a lost and found record by id and copies it into destination.
 *
 * @param id the report id to look up
 * @param destination receives a copy of the record when found
 * @return true when found and copied, false otherwise
 */
bool get_lost_and_found_by_id(id_t id, lost_and_found_record_t *destination);

#endif

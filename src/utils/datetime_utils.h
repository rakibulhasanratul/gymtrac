#ifndef GYMTRAC_DATETIME_UTILS_H
#define GYMTRAC_DATETIME_UTILS_H

#include <stdbool.h>

#include "../types.h"

/**
 * Returns the current wall-clock datetime with second precision.
 *
 * time(NULL) counts UTC seconds since the epoch; shifting by
 * TIMEZONE_OFFSET_HOURS yields local time.
 *
 * @return the current local datetime
 */
datetime_t now_datetime();

/**
 * Converts a datetime into seconds counted from the epoch instant,
 * 1970-01-01 00:00:00. Inverse of datetime_from_seconds().
 *
 * @param datetime_payload the datetime to convert
 * @return seconds elapsed since the epoch
 */
long long datetime_to_seconds(const datetime_t datetime_payload);

/**
 * Converts seconds counted from the epoch instant, 1970-01-01 00:00:00,
 * back into a datetime. Inverse of datetime_to_seconds().
 *
 * @param seconds_since_epoch non-negative seconds elapsed since the epoch
 * @return the matching calendar datetime
 */
datetime_t datetime_from_seconds(long long seconds_since_epoch);

/**
 * Formats a datetime as "yyyy-mm-dd hh:mm:ss" into the caller's buffer.
 *
 * Fails when the buffer is smaller than DATETIME_BUFFER_SIZE.
 *
 * @param datetime_payload the datetime to format
 * @param destination receives the formatted string
 * @param destination_capacity the number of characters destination can hold
 * @return true when the string was written, false otherwise
 */
bool format_datetime(const datetime_t datetime_payload, char *destination, int destination_capacity);

/**
 * Parses a "yyyy-mm-dd hh:mm:ss" string into a datetime.
 *
 * The text must be exactly 19 characters with valid ranges (month 1-12,
 * day valid for the month including leap years, clock parts within their
 * bounds) and a year not before the epoch year.
 *
 * @param datetime_text the string to parse
 * @param destination receives the parsed datetime
 * @return true on success, false on invalid input
 */
bool parse_datetime(const char datetime_text[], datetime_t *destination);

/**
 * Adds days to a datetime, keeping hour, minute, and second unchanged.
 *
 * @param date_payload the starting datetime
 * @param days the number of days to add (can be negative)
 * @return the resulting datetime
 */
datetime_t add_days(const datetime_t date_payload, int days);

/**
 * Adds months to a datetime, clamping the day to the last day of the
 * target month when the original day exceeds it.
 *
 * For example, January 31 plus one month yields February 28 (or 29 in
 * a leap year).
 *
 * @param date_payload the starting datetime
 * @param months the number of months to add (can be negative)
 * @return the resulting datetime
 */
datetime_t add_months(const datetime_t date_payload, int months);

/**
 * Compares two datetimes chronologically.
 *
 * @param left_payload the first datetime
 * @param right_payload the second datetime
 * @return negative when left is earlier, positive when left is later,
 *         zero when both are identical
 */
int compare_datetime(const datetime_t left_payload, const datetime_t right_payload);

/**
 * Calculates the number of whole 24-hour steps between two datetimes.
 *
 * @param earlier_payload the first datetime
 * @param later_payload the second datetime
 * @return the number of whole days from earlier to later, negative when
 *         later precedes earlier
 */
int days_between(const datetime_t earlier_payload, const datetime_t later_payload);

/**
 * Checks whether a datetime equals the EMPTY_DATETIME sentinel, meaning
 * "nothing recorded yet".
 *
 * The sentinel is epoch zero (1970-01-01 00:00:00), matching an unrecorded
 * datetime in data files.
 *
 * @param datetime_payload the datetime to inspect
 * @return true when the datetime matches EMPTY_DATETIME
 */
bool is_empty_datetime(const datetime_t datetime_payload);

#endif

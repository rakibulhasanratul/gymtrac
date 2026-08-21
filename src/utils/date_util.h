#ifndef GYMTRAC_DATE_UTIL_H
#define GYMTRAC_DATE_UTIL_H

#include <stdbool.h>
#include <time.h>

/**
 * Converts a time_t timestamp to a yyyy-mm-dd date string.
 *
 * The timestamp is normalized to whole days (local midnight) before
 * formatting. Fails when the buffer is smaller than DATE_BUFFER_SIZE.
 *
 * @param timestamp the time value to convert
 * @param buffer receives the formatted date string
 * @param buffer_capacity the number of characters buffer can hold;
 *                        must be at least DATE_BUFFER_SIZE
 * @return true when the date was written, false on invalid input
 */
bool time_t_to_string(time_t timestamp, char *buffer, int buffer_capacity);

/**
 * Parses a yyyy-mm-dd date string into a time_t value.
 *
 * The returned time_t is normalized to local midnight. The string must
 * contain exactly ten characters in yyyy-mm-dd format with valid ranges
 * (month 01-12, day 01-31 matching the month, including leap years).
 *
 * @param date_string the string to parse
 * @return the parsed time_t value, or -1 on invalid input
 */
time_t string_to_time_t(const char *date_string);

/**
 * Adds months to a date, clamping the day to the last day of the target
 * month when the original day exceeds it.
 *
 * For example, January 31 plus one month yields February 28 (or 29 in
 * a leap year). The timestamp is normalized to midnight before and
 * after the arithmetic.
 *
 * @param date the starting date
 * @param months the number of months to add (can be negative)
 * @return the resulting time_t value, normalized to midnight
 */
time_t add_months(time_t date, int months);

/**
 * Returns the current system date normalized to local midnight.
 *
 * @return today's date as a day-normalized time_t value
 */
time_t get_today(void);

/**
 * Calculates the number of whole days between two dates.
 *
 * Both timestamps are normalized to midnight before the calculation.
 *
 * @param earlier the earlier date
 * @param later the later date
 * @return the number of whole days from earlier to later, negative when
 *         later precedes earlier
 */
int days_between(time_t earlier, time_t later);

#endif

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "../../src/settings.h"
#include "../../src/types.h"
#include "../../src/utils/datetime_utils.h"
#include "test_datetime_utils.h"

/**
 * Helper: builds a datetime from explicit calendar components.
 */
static datetime_t make_datetime(int year, int month, int day, int hour, int minute, int second)
{
  datetime_t result;
  result.year = year;
  result.month = month;
  result.day = day;
  result.hour = hour;
  result.minute = minute;
  result.second = second;
  return result;
}

/**
 * Verifies that format_datetime produces the expected zero-padded text.
 */
void test_format_datetime_writes_expected_text()
{
  char buffer[DATETIME_BUFFER_SIZE];

  assert(format_datetime(make_datetime(2026, 8, 25, 14, 7, 9), buffer, DATETIME_BUFFER_SIZE));
  assert(strcmp(buffer, "2026-08-25 14:07:09") == 0);

  assert(format_datetime(make_datetime(2024, 1, 5, 3, 42, 0), buffer, DATETIME_BUFFER_SIZE));
  assert(strcmp(buffer, "2024-01-05 03:42:00") == 0);

  assert(format_datetime(make_datetime(2000, 12, 31, 23, 59, 59), buffer, DATETIME_BUFFER_SIZE));
  assert(strcmp(buffer, "2000-12-31 23:59:59") == 0);

  assert(format_datetime(make_datetime(1970, 1, 1, 0, 0, 0), buffer, DATETIME_BUFFER_SIZE));
  assert(strcmp(buffer, "1970-01-01 00:00:00") == 0);
}

/**
 * Verifies that format_datetime rejects a NULL buffer and a too-small one.
 */
void test_format_datetime_rejects_invalid_arguments()
{
  char buffer[DATETIME_BUFFER_SIZE];

  assert(!format_datetime(make_datetime(2024, 1, 15, 10, 0, 0), buffer, DATETIME_BUFFER_SIZE - 1));
  assert(!format_datetime(make_datetime(2024, 1, 15, 10, 0, 0), NULL, DATETIME_BUFFER_SIZE));
}

/**
 * Verifies that parse_datetime fills every field from valid text.
 */
void test_parse_datetime_reads_valid_text()
{
  datetime_t result;

  assert(parse_datetime("2026-08-25 14:07:09", &result));
  assert(result.year == 2026);
  assert(result.month == 8);
  assert(result.day == 25);
  assert(result.hour == 14);
  assert(result.minute == 7);
  assert(result.second == 9);

  // Leap day and midnight edge values.
  assert(parse_datetime("2024-02-29 00:00:00", &result));
  assert(result.month == 2);
  assert(result.day == 29);
}

/**
 * Verifies that parse_datetime rejects malformed text and out-of-range
 * components.
 */
void test_parse_datetime_rejects_invalid()
{
  datetime_t result;

  assert(!parse_datetime(NULL, &result));
  assert(!parse_datetime("2026-08-25 14:07:09", NULL));
  assert(!parse_datetime("", &result));
  assert(!parse_datetime("2026-08-25", &result));
  assert(!parse_datetime("2026/08/25 14:07:09", &result));
  assert(!parse_datetime("2026-08-25T14:07:09", &result));
  assert(!parse_datetime("26-08-25 14:07:09.09", &result));
  assert(!parse_datetime("2026-13-01 00:00:00", &result));
  assert(!parse_datetime("2026-00-01 00:00:00", &result));
  assert(!parse_datetime("2026-04-31 00:00:00", &result));
  assert(!parse_datetime("2023-02-29 00:00:00", &result));
  assert(!parse_datetime("1969-12-31 23:59:59", &result));
  assert(!parse_datetime("2026-08-25 24:00:00", &result));
  assert(!parse_datetime("2026-08-25 12:60:00", &result));
  assert(!parse_datetime("2026-08-25 12:00:60", &result));
}

/**
 * Verifies that formatting then parsing reproduces the original fields.
 */
void test_format_parse_round_trip()
{
  char buffer[DATETIME_BUFFER_SIZE];
  datetime_t original;
  datetime_t parsed;

  original = make_datetime(1999, 12, 31, 23, 59, 59);
  assert(format_datetime(original, buffer, DATETIME_BUFFER_SIZE));
  assert(parse_datetime(buffer, &parsed));
  assert(compare_datetime(original, parsed) == 0);

  original = make_datetime(2024, 2, 29, 6, 30, 15);
  assert(format_datetime(original, buffer, DATETIME_BUFFER_SIZE));
  assert(parse_datetime(buffer, &parsed));
  assert(compare_datetime(original, parsed) == 0);

  original = make_datetime(2030, 7, 4, 18, 5, 2);
  assert(format_datetime(original, buffer, DATETIME_BUFFER_SIZE));
  assert(parse_datetime(buffer, &parsed));
  assert(compare_datetime(original, parsed) == 0);
}

/**
 * Verifies datetime_to_seconds against independently computed epoch
 * second counts.
 */
void test_datetime_to_seconds_known_values()
{
  assert(datetime_to_seconds(make_datetime(1970, 1, 1, 0, 0, 0)) == 0);
  assert(datetime_to_seconds(make_datetime(1970, 1, 2, 0, 0, 0)) == 86400);
  assert(datetime_to_seconds(make_datetime(1972, 1, 1, 0, 0, 0)) == 63072000);
  assert(datetime_to_seconds(make_datetime(2000, 3, 1, 0, 0, 0)) == 951868800LL);
  assert(datetime_to_seconds(make_datetime(2024, 2, 29, 12, 30, 45)) == 1709209845LL);
  assert(datetime_to_seconds(make_datetime(2025, 1, 1, 0, 0, 0)) == 1735689600LL);
  assert(datetime_to_seconds(make_datetime(2026, 8, 25, 14, 7, 9)) == 1787666829LL);
}

/**
 * Verifies that datetime_from_seconds inverts datetime_to_seconds across
 * several timestamps.
 */
void test_seconds_round_trip()
{
  datetime_t original;
  datetime_t restored;
  long long seconds;

  original = make_datetime(2024, 2, 29, 12, 30, 45);
  seconds = datetime_to_seconds(original);
  restored = datetime_from_seconds(seconds);
  assert(compare_datetime(original, restored) == 0);

  original = make_datetime(1970, 1, 1, 0, 0, 0);
  seconds = datetime_to_seconds(original);
  restored = datetime_from_seconds(seconds);
  assert(compare_datetime(original, restored) == 0);

  original = make_datetime(2026, 8, 25, 0, 0, 1);
  seconds = datetime_to_seconds(original);
  restored = datetime_from_seconds(seconds);
  assert(compare_datetime(original, restored) == 0);

  original = make_datetime(2100, 6, 15, 18, 45, 30);
  seconds = datetime_to_seconds(original);
  restored = datetime_from_seconds(seconds);
  assert(compare_datetime(original, restored) == 0);
}

/**
 * Verifies leap year rules through parsing and day arithmetic around
 * February 29.
 */
void test_leap_year_handling()
{
  datetime_t result;

  // 2024 is a leap year (divisible by 4, not by 100).
  assert(parse_datetime("2024-02-29 00:00:00", &result));

  // 2000 is a leap year (divisible by 400).
  assert(parse_datetime("2000-02-29 00:00:00", &result));

  // 2023 and 1900 are not leap years.
  assert(!parse_datetime("2023-02-29 00:00:00", &result));
  assert(!parse_datetime("1900-02-29 00:00:00", &result));

  // One day after Feb 28 lands on Feb 29 only in leap years.
  result = add_days(make_datetime(2024, 2, 28, 22, 0, 0), 1);
  assert(result.day == 29 && result.month == 2);

  result = add_days(make_datetime(2023, 2, 28, 22, 0, 0), 1);
  assert(result.day == 1 && result.month == 3);
}

/**
 * Verifies that add_days rolls over months and years while keeping the
 * clock time.
 */
void test_add_days_crosses_month_and_year_boundaries()
{
  datetime_t result;

  // January 31 + 1 day = February 1.
  result = add_days(make_datetime(2024, 1, 31, 9, 15, 30), 1);
  assert(result.month == 2 && result.day == 1);
  assert(result.hour == 9 && result.minute == 15 && result.second == 30);

  // December 31 + 1 day = January 1 of the next year.
  result = add_days(make_datetime(2024, 12, 31, 23, 59, 59), 1);
  assert(result.year == 2025 && result.month == 1 && result.day == 1);

  // Negative days walk back across the year boundary.
  result = add_days(make_datetime(2025, 1, 1, 0, 0, 0), -1);
  assert(result.year == 2024 && result.month == 12 && result.day == 31);

  // Zero days returns an identical datetime.
  result = add_days(make_datetime(2026, 8, 25, 14, 7, 9), 0);
  assert(compare_datetime(result, make_datetime(2026, 8, 25, 14, 7, 9)) == 0);
}

/**
 * Verifies that add_months clamps the day to the last day of the target
 * month when the original day exceeds it.
 */
void test_add_months_clamps_day()
{
  datetime_t result;

  // January 31 + 1 month = February 28 (2023, non-leap year).
  result = add_months(make_datetime(2023, 1, 31, 10, 20, 30), 1);
  assert(result.year == 2023 && result.month == 2 && result.day == 28);

  // January 31 + 1 month = February 29 (2024, leap year).
  result = add_months(make_datetime(2024, 1, 31, 10, 20, 30), 1);
  assert(result.year == 2024 && result.month == 2 && result.day == 29);

  // March 31 + 1 month = April 30.
  result = add_months(make_datetime(2024, 3, 31, 0, 0, 0), 1);
  assert(result.month == 4 && result.day == 30);

  // May 31 + 1 month = June 30.
  result = add_months(make_datetime(2024, 5, 31, 0, 0, 0), 1);
  assert(result.month == 6 && result.day == 30);
}

/**
 * Verifies that add_months carries into and borrows from adjacent years
 * while preserving the day when it fits.
 */
void test_add_months_year_boundary()
{
  datetime_t result;

  // December 15 + 1 month = January 15 next year.
  result = add_months(make_datetime(2024, 12, 15, 8, 0, 0), 1);
  assert(result.year == 2025 && result.month == 1 && result.day == 15);

  // January 15 - 1 month = December 15 previous year.
  result = add_months(make_datetime(2024, 1, 15, 8, 0, 0), -1);
  assert(result.year == 2023 && result.month == 12 && result.day == 15);

  // January 31 + 12 months = January 31 next year, same clock time.
  result = add_months(make_datetime(2024, 1, 31, 21, 33, 7), 12);
  assert(result.year == 2025 && result.month == 1 && result.day == 31);
  assert(result.hour == 21 && result.minute == 33 && result.second == 7);

  // June 10 + 3 months = September 10.
  result = add_months(make_datetime(2024, 6, 10, 0, 0, 0), 3);
  assert(result.month == 9 && result.day == 10);
}

/**
 * Verifies compare_datetime orders by year, then month, day, hour,
 * minute, and second.
 */
void test_compare_datetime_orders_fields()
{
  datetime_t earlier = make_datetime(2026, 8, 25, 14, 7, 9);
  datetime_t identical = make_datetime(2026, 8, 25, 14, 7, 9);

  assert(compare_datetime(earlier, identical) == 0);

  assert(compare_datetime(make_datetime(2025, 12, 31, 23, 59, 59), earlier) < 0);
  assert(compare_datetime(earlier, make_datetime(2025, 12, 31, 23, 59, 59)) > 0);

  assert(compare_datetime(make_datetime(2026, 7, 25, 14, 7, 9), earlier) < 0);
  assert(compare_datetime(make_datetime(2026, 9, 25, 14, 7, 9), earlier) > 0);

  assert(compare_datetime(make_datetime(2026, 8, 24, 14, 7, 9), earlier) < 0);
  assert(compare_datetime(make_datetime(2026, 8, 26, 14, 7, 9), earlier) > 0);

  assert(compare_datetime(make_datetime(2026, 8, 25, 13, 7, 9), earlier) < 0);
  assert(compare_datetime(make_datetime(2026, 8, 25, 15, 7, 9), earlier) > 0);

  assert(compare_datetime(make_datetime(2026, 8, 25, 14, 6, 9), earlier) < 0);
  assert(compare_datetime(make_datetime(2026, 8, 25, 14, 8, 9), earlier) > 0);

  assert(compare_datetime(make_datetime(2026, 8, 25, 14, 7, 8), earlier) < 0);
  assert(compare_datetime(make_datetime(2026, 8, 25, 14, 7, 10), earlier) > 0);
}

/**
 * Verifies days_between counts whole 24-hour steps, including negative
 * direction.
 */
void test_days_between_calculates_difference()
{
  datetime_t base = make_datetime(2024, 1, 1, 0, 0, 0);

  assert(days_between(base, base) == 0);
  assert(days_between(base, make_datetime(2024, 1, 2, 0, 0, 0)) == 1);
  assert(days_between(base, make_datetime(2024, 1, 31, 0, 0, 0)) == 30);
  assert(days_between(base, make_datetime(2024, 2, 1, 0, 0, 0)) == 31);
  assert(days_between(base, make_datetime(2025, 1, 1, 0, 0, 0)) == 366);

  // Partial days truncate toward whole steps.
  assert(days_between(base, make_datetime(2024, 1, 2, 23, 59, 59)) == 1);

  // Later-first arguments yield negative results.
  assert(days_between(make_datetime(2024, 1, 2, 0, 0, 0), base) == -1);
}

/**
 * Verifies now_datetime produces a plausible current time and moves
 * forward monotonically.
 */
void test_now_datetime_returns_current_time()
{
  datetime_t first = now_datetime();
  datetime_t second = now_datetime();

  // Sanity floor well below any real present date but above the epoch.
  assert(first.year >= 2024);

  // Two consecutive reads never move backward.
  assert(compare_datetime(first, second) <= 0);

  char buffer[DATETIME_BUFFER_SIZE];
  assert(format_datetime(second, buffer, DATETIME_BUFFER_SIZE));
  assert(strlen(buffer) == DATETIME_BUFFER_SIZE - 1);
}

/**
 * Verifies is_empty_datetime recognizes the sentinel and that it survives
 * a storage round-trip through epoch seconds.
 */
void test_is_empty_datetime_checks_all_fields()
{
  assert(is_empty_datetime(EMPTY_DATETIME));
  assert(is_empty_datetime(datetime_from_seconds(0)));

  assert(!is_empty_datetime(make_datetime(2026, 8, 25, 14, 7, 9)));
  assert(!is_empty_datetime(make_datetime(1970, 1, 1, 0, 0, 1)));
}

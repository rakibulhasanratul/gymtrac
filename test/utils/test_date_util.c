#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "../../src/settings.h"
#include "../../src/utils/date_util.h"
#include "test_date_util.h"

/**
 * Helper: builds a time_t from year, month, day (1-indexed) normalized
 * to midnight UTC.
 */
static time_t make_date(int year, int month, int day)
{
  struct tm info;
  memset(&info, 0, sizeof(info));
  info.tm_year = year - 1900;
  info.tm_mon = month - 1;
  info.tm_mday = day;
  return mktime(&info);
}

/**
 * Verifies that time_t_to_string produces the expected yyyy-mm-dd output.
 */
static void test_time_t_to_string_formats_date()
{
  char buffer[DATE_BUFFER_SIZE];

  assert(time_t_to_string(make_date(2024, 1, 15), buffer, DATE_BUFFER_SIZE));
  assert(strcmp(buffer, "2024-01-15") == 0);

  assert(time_t_to_string(make_date(2023, 12, 31), buffer, DATE_BUFFER_SIZE));
  assert(strcmp(buffer, "2023-12-31") == 0);

  assert(time_t_to_string(make_date(2000, 6, 1), buffer, DATE_BUFFER_SIZE));
  assert(strcmp(buffer, "2000-06-01") == 0);
}

/**
 * Verifies that time_t_to_string returns false on invalid input.
 */
static void test_time_t_to_string_invalid_input()
{
  char buffer[DATE_BUFFER_SIZE];

  assert(!time_t_to_string(make_date(2024, 1, 15), buffer, 5));
  assert(!time_t_to_string(make_date(2024, 1, 15), NULL, DATE_BUFFER_SIZE));
}

/**
 * Verifies that string_to_time_t parses valid yyyy-mm-dd strings.
 */
static void test_string_to_time_t_parses_valid_dates()
{
  time_t result;

  result = string_to_time_t("2024-01-15");
  assert(result != (time_t)-1);
  assert(result == make_date(2024, 1, 15));

  result = string_to_time_t("2023-12-31");
  assert(result != (time_t)-1);
  assert(result == make_date(2023, 12, 31));

  result = string_to_time_t("2000-02-29");
  assert(result != (time_t)-1);
  assert(result == make_date(2000, 2, 29));
}

/**
 * Verifies that string_to_time_t rejects invalid formats and values.
 */
static void test_string_to_time_t_rejects_invalid()
{
  assert(string_to_time_t(NULL) == (time_t)-1);
  assert(string_to_time_t("") == (time_t)-1);
  assert(string_to_time_t("2024/01/15") == (time_t)-1);
  assert(string_to_time_t("24-01-15") == (time_t)-1);
  assert(string_to_time_t("2024-13-01") == (time_t)-1);
  assert(string_to_time_t("2024-00-01") == (time_t)-1);
  assert(string_to_time_t("2024-01-32") == (time_t)-1);
  assert(string_to_time_t("2024-01-00") == (time_t)-1);
  assert(string_to_time_t("2023-02-29") == (time_t)-1);
  assert(string_to_time_t("not-a-date") == (time_t)-1);
}

/**
 * Verifies that time_t_to_string and string_to_time_t round-trip.
 */
static void test_date_round_trip()
{
  char buffer[DATE_BUFFER_SIZE];
  time_t original;
  time_t parsed;

  original = make_date(2024, 6, 15);
  assert(time_t_to_string(original, buffer, DATE_BUFFER_SIZE));
  parsed = string_to_time_t(buffer);
  assert(parsed == original);

  original = make_date(2000, 2, 29);
  assert(time_t_to_string(original, buffer, DATE_BUFFER_SIZE));
  parsed = string_to_time_t(buffer);
  assert(parsed == original);

  original = make_date(1999, 12, 31);
  assert(time_t_to_string(original, buffer, DATE_BUFFER_SIZE));
  parsed = string_to_time_t(buffer);
  assert(parsed == original);
}

/**
 * Verifies that leap year dates parse correctly.
 */
static void test_leap_year_dates()
{
  // 2024 is a leap year (divisible by 4, not by 100).
  time_t feb29_2024 = string_to_time_t("2024-02-29");
  assert(feb29_2024 != (time_t)-1);
  assert(feb29_2024 == make_date(2024, 2, 29));

  // 2023 is not a leap year.
  assert(string_to_time_t("2023-02-29") == (time_t)-1);

  // 2000 is a leap year (divisible by 400).
  time_t feb29_2000 = string_to_time_t("2000-02-29");
  assert(feb29_2000 != (time_t)-1);
  assert(feb29_2000 == make_date(2000, 2, 29));

  // 1900 is not a leap year (divisible by 100 but not 400).
  assert(string_to_time_t("1900-02-29") == (time_t)-1);
}

/**
 * Verifies that add_months clamps the day to the last day of the target
 * month when the original day exceeds it.
 */
static void test_add_months_clamps_day()
{
  time_t result;
  char buffer[DATE_BUFFER_SIZE];

  // January 31 + 1 month = February 28 (2023, non-leap year).
  result = add_months(make_date(2023, 1, 31), 1);
  time_t_to_string(result, buffer, DATE_BUFFER_SIZE);
  assert(strcmp(buffer, "2023-02-28") == 0);

  // January 31 + 1 month = February 29 (2024, leap year).
  result = add_months(make_date(2024, 1, 31), 1);
  time_t_to_string(result, buffer, DATE_BUFFER_SIZE);
  assert(strcmp(buffer, "2024-02-29") == 0);

  // March 31 + 1 month = April 30.
  result = add_months(make_date(2024, 3, 31), 1);
  time_t_to_string(result, buffer, DATE_BUFFER_SIZE);
  assert(strcmp(buffer, "2024-04-30") == 0);

  // May 31 + 1 month = June 30.
  result = add_months(make_date(2024, 5, 31), 1);
  time_t_to_string(result, buffer, DATE_BUFFER_SIZE);
  assert(strcmp(buffer, "2024-06-30") == 0);
}

/**
 * Verifies that add_months handles positive and negative offsets
 * across year boundaries.
 */
static void test_add_months_year_boundary()
{
  time_t result;
  char buffer[DATE_BUFFER_SIZE];

  // December 15 + 1 month = January 15 next year.
  result = add_months(make_date(2024, 12, 15), 1);
  time_t_to_string(result, buffer, DATE_BUFFER_SIZE);
  assert(strcmp(buffer, "2025-01-15") == 0);

  // January 15 - 1 month = December 15 previous year.
  result = add_months(make_date(2024, 1, 15), -1);
  time_t_to_string(result, buffer, DATE_BUFFER_SIZE);
  assert(strcmp(buffer, "2023-12-15") == 0);

  // January 31 + 12 months = January 31 next year.
  result = add_months(make_date(2024, 1, 31), 12);
  time_t_to_string(result, buffer, DATE_BUFFER_SIZE);
  assert(strcmp(buffer, "2025-01-31") == 0);
}

/**
 * Verifies that add_months preserves the day when it fits the target
 * month.
 */
static void test_add_months_preserves_day()
{
  time_t result;
  char buffer[DATE_BUFFER_SIZE];

  // January 15 + 1 month = February 15.
  result = add_months(make_date(2024, 1, 15), 1);
  time_t_to_string(result, buffer, DATE_BUFFER_SIZE);
  assert(strcmp(buffer, "2024-02-15") == 0);

  // June 10 + 3 months = September 10.
  result = add_months(make_date(2024, 6, 10), 3);
  time_t_to_string(result, buffer, DATE_BUFFER_SIZE);
  assert(strcmp(buffer, "2024-09-10") == 0);
}

/**
 * Verifies that days_between calculates the correct number of days.
 */
static void test_days_between_calculates_difference()
{
  assert(days_between(make_date(2024, 1, 1), make_date(2024, 1, 1)) == 0);
  assert(days_between(make_date(2024, 1, 1), make_date(2024, 1, 2)) == 1);
  assert(days_between(make_date(2024, 1, 1), make_date(2024, 1, 31)) == 30);
  assert(days_between(make_date(2024, 1, 1), make_date(2024, 2, 1)) == 31);
  assert(days_between(make_date(2024, 1, 1), make_date(2025, 1, 1)) == 366);

  // Negative direction: later as first argument yields negative.
  assert(days_between(make_date(2024, 1, 2), make_date(2024, 1, 1)) == -1);
}

/**
 * Verifies that add_months with zero months returns the same date.
 */
static void test_add_months_zero_returns_same()
{
  time_t original = make_date(2024, 6, 15);
  time_t result = add_months(original, 0);
  assert(result == original);
}

/**
 * Runs every date_util unit test, aborting on the first failure.
 */
void run_all_date_util_tests()
{
  test_time_t_to_string_formats_date();
  test_time_t_to_string_invalid_input();
  test_string_to_time_t_parses_valid_dates();
  test_string_to_time_t_rejects_invalid();
  test_date_round_trip();
  test_leap_year_dates();
  test_add_months_clamps_day();
  test_add_months_year_boundary();
  test_add_months_preserves_day();
  test_days_between_calculates_difference();
  test_add_months_zero_returns_same();
}

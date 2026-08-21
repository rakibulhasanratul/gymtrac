#include <stdio.h>
#include <string.h>
#include <time.h>

#include "../settings.h"
#include "date_util.h"

// Seconds in a single day.
#define SECONDS_PER_DAY 86400

// Calendar fields extracted from a time_t timestamp.
typedef struct tm calendar_time_t;

// Days in each month (non-leap year).
static const int DAYS_IN_MONTH[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

// Returns whether the given year is a leap year.
static int is_leap_year(int year)
{
  if (year % 400 == 0)
    return 1;
  if (year % 100 == 0)
    return 0;
  if (year % 4 == 0)
    return 1;
  return 0;
}

// Returns the number of days in the given month (1-indexed).
static int days_in_month(int year, int month)
{
  if (month == 2 && is_leap_year(year))
    return 29;
  return DAYS_IN_MONTH[month - 1];
}

// Normalizes a timestamp to midnight of its own calendar day.
static time_t normalize_to_midnight(time_t timestamp)
{
  calendar_time_t calendar_time;
  localtime_r(&timestamp, &calendar_time);
  calendar_time.tm_hour = 0;
  calendar_time.tm_min = 0;
  calendar_time.tm_sec = 0;
  return mktime(&calendar_time);
}

bool time_t_to_string(time_t timestamp, char *buffer, int buffer_capacity)
{
  if (buffer == NULL || buffer_capacity < DATE_BUFFER_SIZE)
    return 0;

  // Extract calendar fields from the timestamp.
  calendar_time_t calendar_time;
  localtime_r(&timestamp, &calendar_time);

  // Format as yyyy-mm-dd into the caller's buffer.
  int written = snprintf(buffer, (size_t)buffer_capacity, "%04d-%02d-%02d", calendar_time.tm_year + 1900,
                         calendar_time.tm_mon + 1, calendar_time.tm_mday);

  // Verify the full 10-character date was written.
  return (written == DATE_BUFFER_SIZE - 1);
}

time_t string_to_time_t(const char *date_string)
{
  if (date_string == NULL)
    return (time_t)-1;

  // Enforce exactly 10 characters in yyyy-mm-dd format.
  int length = (int)strlen(date_string);
  if (length != DATE_BUFFER_SIZE - 1)
    return (time_t)-1;
  if (date_string[4] != '-' || date_string[7] != '-')
    return (time_t)-1;

  int year = 0;
  int month = 0;
  int day = 0;

  if (sscanf(date_string, "%4d-%2d-%2d", &year, &month, &day) != 3)
    return (time_t)-1;

  // Reject out-of-range month and day values.
  if (month < 1 || month > 12)
    return (time_t)-1;
  if (day < 1 || day > days_in_month(year, month))
    return (time_t)-1;

  // Populate a calendar_time_t and convert to a normalized time_t.
  calendar_time_t calendar_time;
  memset(&calendar_time, 0, sizeof(calendar_time));
  calendar_time.tm_year = year - 1900;
  calendar_time.tm_mon = month - 1;
  calendar_time.tm_mday = day;

  time_t result = mktime(&calendar_time);
  return normalize_to_midnight(result);
}

time_t add_months(time_t date, int months)
{
  calendar_time_t calendar_time;
  localtime_r(&date, &calendar_time);

  // Decompose into year and 1-indexed month for easier arithmetic.
  int target_month = calendar_time.tm_mon + 1 + months;
  int target_year = calendar_time.tm_year + 1900;

  // Normalize so month is in range 1-12 with year carry.
  target_year += (target_month - 1) / 12;
  target_month = ((target_month - 1) % 12) + 1;
  if (target_month <= 0)
  {
    target_month += 12;
    target_year--;
  }

  // Clamp the day to the last day of the target month.
  int max_day = days_in_month(target_year, target_month);
  int target_day = calendar_time.tm_mday;
  if (target_day > max_day)
    target_day = max_day;

  // Build the result date and normalize to midnight.
  calendar_time_t result_calendar_time;
  memset(&result_calendar_time, 0, sizeof(result_calendar_time));
  result_calendar_time.tm_year = target_year - 1900;
  result_calendar_time.tm_mon = target_month - 1;
  result_calendar_time.tm_mday = target_day;

  time_t result_time = mktime(&result_calendar_time);
  return normalize_to_midnight(result_time);
}

time_t get_today(void)
{
  // Fetch the current wall-clock time and strip hours/minutes/seconds.
  time_t now = time(NULL);
  return normalize_to_midnight(now);
}

int days_between(time_t earlier, time_t later)
{
  // Normalize both dates to midnight so partial days don't affect the count.
  time_t normalized_earlier = normalize_to_midnight(earlier);
  time_t normalized_later = normalize_to_midnight(later);

  // difftime returns seconds as a double; convert to whole days.
  double diff = difftime(normalized_later, normalized_earlier);
  return (int)(diff / SECONDS_PER_DAY);
}

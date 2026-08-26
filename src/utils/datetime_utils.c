#include <stdio.h>
#include <string.h>
#include <time.h>

#include "../settings.h"
#include "../types.h"
#include "datetime_utils.h"

// Base calendar units; every conversion below is built from these.
#define SECONDS_PER_MINUTE 60
#define MINUTES_PER_HOUR 60
#define HOURS_PER_DAY 24
#define MONTHS_PER_YEAR 12

// Derived units, expressed through the base ones.
#define SECONDS_PER_HOUR (MINUTES_PER_HOUR * SECONDS_PER_MINUTE)
#define SECONDS_PER_DAY (HOURS_PER_DAY * SECONDS_PER_HOUR)

// The instant all epoch second counts are measured from: 1970-01-01 00:00:00.
#define EPOCH_YEAR 1970

// Days in each month for a non-leap year, indexed by month - 1.
static const int DAYS_IN_MONTH[MONTHS_PER_YEAR] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

// Returns whether the given year is a leap year.
static bool is_leap_year(int year)
{
  if (year % 400 == 0) return true;
  if (year % 100 == 0) return false;
  if (year % 4 == 0) return true;
  return false;
}

// Returns the number of days in the given month (1-indexed).
static int days_in_month(int year, int month)
{
  if (month == 2 && is_leap_year(year)) return 29;
  return DAYS_IN_MONTH[month - 1];
}

// Returns the number of days in the given year.
static int days_in_year(int year)
{
  if (is_leap_year(year)) return 366;
  return 365;
}

datetime_t now_datetime()
{
  long long utc_seconds = (long long)time(NULL);
  return datetime_from_seconds(utc_seconds + (long long)TIMEZONE_OFFSET_HOURS * SECONDS_PER_HOUR);
}

long long datetime_to_seconds(const datetime_t datetime_payload)
{
  long long total_seconds = 0;

  // Each full year between EPOCH_YEAR and the target adds its length.
  for (int year = EPOCH_YEAR; year < datetime_payload.year; year++)
    total_seconds += (long long)days_in_year(year) * SECONDS_PER_DAY;

  // Full months before the target add their length, leap-aware for the year.
  for (int month = 1; month < datetime_payload.month; month++)
    total_seconds += (long long)days_in_month(datetime_payload.year, month) * SECONDS_PER_DAY;

  // The rest is the not-yet-counted day plus the plain clock time.
  total_seconds += (long long)(datetime_payload.day - 1) * SECONDS_PER_DAY;
  total_seconds += (long long)datetime_payload.hour * SECONDS_PER_HOUR;
  total_seconds += (long long)datetime_payload.minute * SECONDS_PER_MINUTE;

  return total_seconds + datetime_payload.second;
}

datetime_t datetime_from_seconds(long long seconds_since_epoch)
{
  // Whole days since the epoch; leftover seconds form the final day's clock.
  long long day_count = seconds_since_epoch / SECONDS_PER_DAY;
  long long remaining_seconds = seconds_since_epoch % SECONDS_PER_DAY;

  datetime_t result;
  result.year = EPOCH_YEAR;
  result.month = 1;
  result.day = 1;
  result.hour = (int)(remaining_seconds / SECONDS_PER_HOUR);
  result.minute = (int)(remaining_seconds % SECONDS_PER_HOUR / SECONDS_PER_MINUTE);
  result.second = (int)(remaining_seconds % SECONDS_PER_MINUTE);

  // Peel off one full year at a time until the leftover fits inside one.
  while (day_count >= days_in_year(result.year))
  {
    day_count -= days_in_year(result.year);
    result.year++;
  }

  // Then peel off full months; what remains lands on the day field.
  while (day_count >= days_in_month(result.year, result.month))
  {
    day_count -= days_in_month(result.year, result.month);
    result.month++;
  }

  result.day += (int)day_count;
  return result;
}

bool format_datetime(const datetime_t datetime_payload, char *buffer_destination, int destination_capacity)
{
  if (buffer_destination == NULL || destination_capacity < DATETIME_BUFFER_SIZE) return false;

  snprintf(
    buffer_destination, (size_t)destination_capacity, "%04d-%02d-%02d %02d:%02d:%02d", datetime_payload.year,
    datetime_payload.month, datetime_payload.day, datetime_payload.hour, datetime_payload.minute,
    datetime_payload.second
  );
  return true;
}

bool parse_datetime(const char datetime_text[], datetime_t *datetime_destination)
{
  if (datetime_text == NULL || datetime_destination == NULL) return false;

  // The text must be exactly 19 characters with fixed separators:
  // "yyyy-mm-dd hh:mm:ss".
  if (strlen(datetime_text) != DATETIME_BUFFER_SIZE - 1) return false;
  if (datetime_text[4] != '-' || datetime_text[7] != '-' || datetime_text[10] != ' ') return false;
  if (datetime_text[13] != ':' || datetime_text[16] != ':') return false;

  int year = 0;
  int month = 0;
  int day = 0;
  int hour = 0;
  int minute = 0;
  int second = 0;
  if (sscanf(datetime_text, "%4d-%2d-%2d %2d:%2d:%2d", &year, &month, &day, &hour, &minute, &second) != 6) return false;

  // Reject any component outside its valid range.
  if (year < EPOCH_YEAR || month < 1 || month > MONTHS_PER_YEAR) return false;
  if (day < 1 || day > days_in_month(year, month)) return false;
  if (hour < 0 || hour > HOURS_PER_DAY - 1) return false;
  if (minute < 0 || minute > MINUTES_PER_HOUR - 1) return false;
  if (second < 0 || second > SECONDS_PER_MINUTE - 1) return false;

  datetime_destination->year = year;
  datetime_destination->month = month;
  datetime_destination->day = day;
  datetime_destination->hour = hour;
  datetime_destination->minute = minute;
  datetime_destination->second = second;
  return true;
}

datetime_t add_days(const datetime_t date_payload, int days)
{
  long long shifted_seconds = datetime_to_seconds(date_payload) + (long long)days * SECONDS_PER_DAY;
  return datetime_from_seconds(shifted_seconds);
}

datetime_t add_months(const datetime_t date_payload, int months)
{
  datetime_t result = date_payload;
  result.month += months;

  // Carry or borrow whole years until month is back inside 1-12.
  while (result.month > MONTHS_PER_YEAR)
  {
    result.month -= MONTHS_PER_YEAR;
    result.year++;
  }
  while (result.month < 1)
  {
    result.month += MONTHS_PER_YEAR;
    result.year--;
  }

  // Clamp the day when the target month is shorter than the original one.
  int last_day_of_month = days_in_month(result.year, result.month);
  if (result.day > last_day_of_month) result.day = last_day_of_month;

  return result;
}

int compare_datetime(const datetime_t left_payload, const datetime_t right_payload)
{
  if (left_payload.year < right_payload.year) return -1;
  if (left_payload.year > right_payload.year) return 1;
  if (left_payload.month < right_payload.month) return -1;
  if (left_payload.month > right_payload.month) return 1;
  if (left_payload.day < right_payload.day) return -1;
  if (left_payload.day > right_payload.day) return 1;
  if (left_payload.hour < right_payload.hour) return -1;
  if (left_payload.hour > right_payload.hour) return 1;
  if (left_payload.minute < right_payload.minute) return -1;
  if (left_payload.minute > right_payload.minute) return 1;
  if (left_payload.second < right_payload.second) return -1;
  if (left_payload.second > right_payload.second) return 1;
  return 0;
}

int days_between(const datetime_t earlier_payload, const datetime_t later_payload)
{
  long long difference = datetime_to_seconds(later_payload) - datetime_to_seconds(earlier_payload);
  return (int)(difference / SECONDS_PER_DAY);
}

bool is_empty_datetime(const datetime_t datetime_payload)
{
  return compare_datetime(datetime_payload, EMPTY_DATETIME) == 0;
}

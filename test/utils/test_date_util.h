#ifndef GYMTRAC_TEST_DATE_UTIL_H
#define GYMTRAC_TEST_DATE_UTIL_H

void test_time_t_to_string_formats_date();
void test_time_t_to_string_invalid_input();
void test_string_to_time_t_parses_valid_dates();
void test_string_to_time_t_rejects_invalid();
void test_date_round_trip();
void test_leap_year_dates();
void test_add_months_clamps_day();
void test_add_months_year_boundary();
void test_add_months_preserves_day();
void test_days_between_calculates_difference();
void test_add_months_zero_returns_same();

#endif

#ifndef GYMTRAC_TEST_DATETIME_UTILS_H
#define GYMTRAC_TEST_DATETIME_UTILS_H

void test_format_datetime_writes_expected_text();
void test_format_datetime_rejects_invalid_arguments();
void test_parse_datetime_reads_valid_text();
void test_parse_datetime_rejects_invalid();
void test_format_parse_round_trip();
void test_datetime_to_seconds_known_values();
void test_seconds_round_trip();
void test_leap_year_handling();
void test_add_days_crosses_month_and_year_boundaries();
void test_add_months_clamps_day();
void test_add_months_year_boundary();
void test_add_months_preserves_day();
void test_compare_datetime_orders_fields();
void test_days_between_calculates_difference();
void test_now_datetime_returns_current_time();
void test_is_empty_datetime_checks_all_fields();

#endif

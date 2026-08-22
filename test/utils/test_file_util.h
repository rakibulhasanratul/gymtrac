#ifndef GYMTRAC_TEST_FILE_UTIL_H
#define GYMTRAC_TEST_FILE_UTIL_H

void test_write_read_round_trip();
void test_read_tolerates_crlf_line_ending();
void test_read_drains_overlong_line();
void test_read_line_rejects_invalid_arguments();
void test_write_line_rejects_invalid_arguments();
void test_write_read_lines_round_trip();
void test_read_lines_skip_empty_lines();
void test_write_lines_overwrite_existing_content();
void test_read_lines_respects_max_lines();
void test_read_lines_missing_file_returns_zero();
void test_write_lines_rejects_invalid_arguments();
void test_read_lines_rejects_invalid_arguments();

#endif

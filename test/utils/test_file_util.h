#ifndef GYMTRAC_TEST_FILE_UTIL_H
#define GYMTRAC_TEST_FILE_UTIL_H

void test_write_read_round_trip();
void test_read_tolerates_crlf_line_ending();
void test_read_drains_overlong_line();
void test_read_line_rejects_invalid_arguments();
void test_write_line_rejects_invalid_arguments();
void test_build_file_path_joins_correctly();
void test_build_file_path_null_inputs_are_safe();
void test_build_file_path_overflow_produces_empty();

#endif

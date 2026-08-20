#ifndef GYMTRAC_TEST_INPUT_H
#define GYMTRAC_TEST_INPUT_H

void test_input_string_reads_plain_lines();
void test_input_string_tolerates_crlf();
void test_input_string_caps_and_drains_overlong();
void test_input_string_reads_empty_line();
void test_input_string_rejects_invalid_arguments();
void test_input_integer_reads_valid_values();
void test_input_integer_rejects_bad_input();
void test_input_integer_drains_remaining_line();
void test_input_positive_int_keeps_positives_only();

#endif

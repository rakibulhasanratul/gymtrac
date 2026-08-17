#include <assert.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "../src/utils/input.h"
#include "test_input.h"

#define TEST_INPUT_PATH "tmp_test_input.dat"

/**
 * Redirects standard input to a temporary file holding content.
 */
static void redirect_stdin_with_content(const char content[])
{
  FILE *file;

  file = fopen(TEST_INPUT_PATH, "w");
  assert(file != NULL);
  assert(fputs(content, file) >= 0);
  assert(fclose(file) == 0);

  file = freopen(TEST_INPUT_PATH, "r", stdin);
  assert(file != NULL);
}

/**
 * Verifies that input_string reads lines stripped of the newline.
 */
static void test_input_string_reads_plain_lines(void)
{
  char buffer[64];

  redirect_stdin_with_content("hello world\nsecond\n");
  assert(input_string(buffer, sizeof(buffer)) == true);
  assert(strcmp(buffer, "hello world") == 0);
  assert(input_string(buffer, sizeof(buffer)) == true);
  assert(strcmp(buffer, "second") == 0);
  assert(input_string(buffer, sizeof(buffer)) == false);
}

/**
 * Verifies that input_string tolerates CRLF line endings.
 */
static void test_input_string_tolerates_crlf(void)
{
  char buffer[64];

  redirect_stdin_with_content("foo\r\nbar\n");
  assert(input_string(buffer, sizeof(buffer)) == true);
  assert(strcmp(buffer, "foo") == 0);
  assert(input_string(buffer, sizeof(buffer)) == true);
  assert(strcmp(buffer, "bar") == 0);
}

/**
 * Verifies that an over-long line is capped and its remainder drained.
 */
static void test_input_string_caps_and_drains_overlong(void)
{
  char buffer[8];

  redirect_stdin_with_content("toolonginput\nnext\n");
  assert(input_string(buffer, sizeof(buffer)) == true);
  assert(strcmp(buffer, "toolong") == 0);
  assert(input_string(buffer, sizeof(buffer)) == true);
  assert(strcmp(buffer, "next") == 0);
}

/**
 * Verifies that an empty line reads back as an empty string.
 */
static void test_input_string_reads_empty_line(void)
{
  char buffer[64];

  redirect_stdin_with_content("\n");
  assert(input_string(buffer, sizeof(buffer)) == true);
  assert(strcmp(buffer, "") == 0);
}

/**
 * Verifies that input_string rejects invalid arguments.
 */
static void test_input_string_rejects_invalid_arguments(void)
{
  char buffer[64];

  redirect_stdin_with_content("anything\n");
  assert(input_string(NULL, sizeof(buffer)) == false);
  assert(input_string(buffer, 1) == false);
}

/**
 * Verifies that input_integer reads positives, negatives, and zero.
 */
static void test_input_integer_reads_valid_values(void)
{
  int value;

  redirect_stdin_with_content("42\n-7\n0\n2147483647\n");
  assert(input_integer(&value) == true);
  assert(value == 42);
  assert(input_integer(&value) == true);
  assert(value == -7);
  assert(input_integer(&value) == true);
  assert(value == 0);
  assert(input_integer(&value) == true);
  assert(value == INT_MAX);
  assert(input_integer(&value) == false);
}

/**
 * Verifies that input_integer rejects non-numeric input and invalid arguments.
 */
static void test_input_integer_rejects_bad_input(void)
{
  int value;

  redirect_stdin_with_content("abc\n");
  assert(input_integer(&value) == false);

  redirect_stdin_with_content("");
  assert(input_integer(&value) == false);

  assert(input_integer(NULL) == false);
}

/**
 * Verifies that the remainder of a number line is drained for later reads.
 */
static void test_input_integer_drains_remaining_line(void)
{
  int value;
  char buffer[64];

  redirect_stdin_with_content("42 junk\nnext\n");
  assert(input_integer(&value) == true);
  assert(value == 42);
  assert(input_string(buffer, sizeof(buffer)) == true);
  assert(strcmp(buffer, "next") == 0);
}

/**
 * Verifies that input_positive_int keeps positives and rejects everything else.
 */
static void test_input_positive_int_keeps_positives_only(void)
{
  int value;

  redirect_stdin_with_content("5\n0\n-3\nabc\n");
  assert(input_positive_int(&value) == true);
  assert(value == 5);
  assert(input_positive_int(&value) == false);
  assert(input_positive_int(&value) == false);
  assert(input_positive_int(&value) == false);
  assert(input_positive_int(NULL) == false);
}

/**
 * Runs every input unit test, aborting on the first failure.
 */
void run_all_input_tests(void)
{
  test_input_string_reads_plain_lines();
  test_input_string_tolerates_crlf();
  test_input_string_caps_and_drains_overlong();
  test_input_string_reads_empty_line();
  test_input_string_rejects_invalid_arguments();
  test_input_integer_reads_valid_values();
  test_input_integer_rejects_bad_input();
  test_input_integer_drains_remaining_line();
  test_input_positive_int_keeps_positives_only();
}

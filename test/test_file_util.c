#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "../src/utils/file_util.h"
#include "test_file_util.h"

#define TEST_FILE_PATH "tmp_test_file_util.dat"

/**
 * Writes the given lines to the test file and returns the open file.
 */
static FILE *open_test_file_for_write(void) {
  FILE *file = fopen(TEST_FILE_PATH, "w");
  assert(file != NULL);
  return file;
}

/**
 * Opens the test file for reading and returns the open file.
 */
static FILE *open_test_file_for_read(void) {
  FILE *file = fopen(TEST_FILE_PATH, "r");
  assert(file != NULL);
  return file;
}

/**
 * Verifies that lines written with file_write_line read back unchanged.
 */
static void test_write_read_round_trip(void) {
  FILE *file;
  char buffer[128];

  file = open_test_file_for_write();
  assert(file_write_line(file, "alpha|beta"));
  assert(file_write_line(file, "single"));
  assert(file_write_line(file, ""));
  fclose(file);

  file = open_test_file_for_read();
  assert(file_read_line(file, buffer, sizeof(buffer)));
  assert(strcmp(buffer, "alpha|beta") == 0);
  assert(file_read_line(file, buffer, sizeof(buffer)));
  assert(strcmp(buffer, "single") == 0);
  assert(file_read_line(file, buffer, sizeof(buffer)));
  assert(strcmp(buffer, "") == 0);
  assert(file_read_line(file, buffer, sizeof(buffer)) == false);
  fclose(file);
}

/**
 * Verifies that reading tolerates CRLF line endings.
 */
static void test_read_crlf_line_ending(void) {
  FILE *file;
  char buffer[128];

  file = open_test_file_for_write();
  fputs("foo\r\n", file);
  fputs("bar\n", file);
  fclose(file);

  file = open_test_file_for_read();
  assert(file_read_line(file, buffer, sizeof(buffer)));
  assert(strcmp(buffer, "foo") == 0);
  assert(file_read_line(file, buffer, sizeof(buffer)));
  assert(strcmp(buffer, "bar") == 0);
  fclose(file);
}

/**
 * Verifies that an over-long line is drained and never read as records.
 */
static void test_read_truncated_line_drained(void) {
  FILE *file;
  char buffer[8];

  file = open_test_file_for_write();
  assert(file_write_line(file, "alpha|beta"));
  assert(file_write_line(file, "next"));
  fclose(file);

  file = open_test_file_for_read();
  assert(file_read_line(file, buffer, sizeof(buffer)) == false);
  assert(file_read_line(file, buffer, sizeof(buffer)));
  assert(strcmp(buffer, "next") == 0);
  fclose(file);
}

/**
 * Verifies that file_read_line rejects invalid arguments.
 */
static void test_read_line_invalid_arguments(void) {
  char buffer[64];

  assert(file_read_line(NULL, buffer, sizeof(buffer)) == false);
  assert(file_read_line(stdin, NULL, sizeof(buffer)) == false);
  assert(file_read_line(stdin, buffer, 1) == false);
}

/**
 * Verifies that file_write_line rejects invalid arguments.
 */
static void test_write_line_invalid_arguments(void) {
  assert(file_write_line(NULL, "line") == false);
  assert(file_write_line(stdout, NULL) == false);
}

/**
 * Verifies that file_sanitize_field strips control characters and delimiters.
 */
static void test_sanitize_field(void) {
  char buffer[128];

  strcpy(buffer, "bad\t|\nchar\x01s\r");
  assert(file_sanitize_field(buffer) == buffer);
  assert(strcmp(buffer, "badchars") == 0);

  strcpy(buffer, "clean|text");
  assert(file_sanitize_field(buffer) == buffer);
  assert(strcmp(buffer, "cleantext") == 0);

  strcpy(buffer, "no change needed");
  assert(file_sanitize_field(buffer) == buffer);
  assert(strcmp(buffer, "no change needed") == 0);

  strcpy(buffer, "||||");
  assert(file_sanitize_field(buffer) == buffer);
  assert(strcmp(buffer, "") == 0);

  assert(file_sanitize_field(NULL) == NULL);
}

/**
 * Runs every file_util unit test, aborting on the first failure.
 */
void run_file_util_tests(void) {
  test_write_read_round_trip();
  test_read_crlf_line_ending();
  test_read_truncated_line_drained();
  test_read_line_invalid_arguments();
  test_write_line_invalid_arguments();
  test_sanitize_field();
}

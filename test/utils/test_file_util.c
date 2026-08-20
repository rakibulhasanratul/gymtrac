#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "../../src/utils/file_util.h"
#include "test_file_util.h"
#include "../../src/settings.h"

#define TEST_FILE_PATH  DEFAULT_DATA_DIRECTORY"/tmp_test_file_util.dat"

/**
 * Writes the given lines to the test file and returns the open file.
 */
static FILE *open_test_file_for_write()
{
  FILE *file = fopen(TEST_FILE_PATH, "w");
  assert(file != NULL);
  return file;
}

/**
 * Opens the test file for reading and returns the open file.
 */
static FILE *open_test_file_for_read()
{
  FILE *file = fopen(TEST_FILE_PATH, "r");
  assert(file != NULL);
  return file;
}

/**
 * Verifies that lines written with write_line_to_file read back unchanged.
 */
static void test_write_read_round_trip()
{
  FILE *file;
  char buffer[128];

  file = open_test_file_for_write();
  assert(write_line_to_file(file, "alpha|beta"));
  assert(write_line_to_file(file, "single"));
  assert(write_line_to_file(file, ""));
  fclose(file);

  file = open_test_file_for_read();
  assert(read_line_from_file(file, buffer, sizeof(buffer)));
  assert(strcmp(buffer, "alpha|beta") == 0);
  assert(read_line_from_file(file, buffer, sizeof(buffer)));
  assert(strcmp(buffer, "single") == 0);
  assert(read_line_from_file(file, buffer, sizeof(buffer)));
  assert(strcmp(buffer, "") == 0);
  assert(read_line_from_file(file, buffer, sizeof(buffer)) == false);
  fclose(file);
}

/**
 * Verifies that reading tolerates CRLF line endings.
 */
static void test_read_tolerates_crlf_line_ending()
{
  FILE *file;
  char buffer[128];

  file = open_test_file_for_write();
  fputs("foo\r\n", file);
  fputs("bar\n", file);
  fclose(file);

  file = open_test_file_for_read();
  assert(read_line_from_file(file, buffer, sizeof(buffer)));
  assert(strcmp(buffer, "foo") == 0);
  assert(read_line_from_file(file, buffer, sizeof(buffer)));
  assert(strcmp(buffer, "bar") == 0);
  fclose(file);
}

/**
 * Verifies that an over-long line is drained and never read as records.
 */
static void test_read_drains_overlong_line()
{
  FILE *file;
  char buffer[8];

  file = open_test_file_for_write();
  assert(write_line_to_file(file, "alpha|beta"));
  assert(write_line_to_file(file, "next"));
  fclose(file);

  file = open_test_file_for_read();
  assert(read_line_from_file(file, buffer, sizeof(buffer)) == false);
  assert(read_line_from_file(file, buffer, sizeof(buffer)));
  assert(strcmp(buffer, "next") == 0);
  fclose(file);
}

/**
 * Verifies that read_line_from_file rejects invalid arguments.
 */
static void test_read_line_rejects_invalid_arguments()
{
  char buffer[64];

  assert(read_line_from_file(NULL, buffer, sizeof(buffer)) == false);
  assert(read_line_from_file(stdin, NULL, sizeof(buffer)) == false);
  assert(read_line_from_file(stdin, buffer, 1) == false);
}

/**
 * Verifies that write_line_to_file rejects invalid arguments.
 */
static void test_write_line_rejects_invalid_arguments()
{
  assert(write_line_to_file(NULL, "line") == false);
  assert(write_line_to_file(stdout, NULL) == false);
}

/**
 * Verifies that build_file_path correctly joins a directory and filename.
 */
static void test_build_file_path_joins_correctly()
{
  char path[260];
  build_file_path("branches.txt", path, sizeof(path));
  assert(strcmp(path, DEFAULT_DATA_DIRECTORY"/branches.txt") == 0);
}

/**
 * Verifies that build_file_path handles NULL inputs without crashing.
 */
static void test_build_file_path_null_inputs_are_safe()
{
  char path[260];
  path[0] = '\0';

  build_file_path(NULL, path, sizeof(path));
  assert(strlen(path) == 0);

  build_file_path("file.txt", NULL, 0);
}

/**
 * Verifies that build_file_path produces an empty string on overflow.
 */
static void test_build_file_path_overflow_produces_empty()
{
  char long_directory[260];
  int index;

  for (index = 0; index < 257; index++)
    long_directory[index] = 'a';
  long_directory[index] = '\0';

  char path[260];
  build_file_path(long_directory, path, sizeof(path));
  assert(strlen(path) == 0);
}

/**
 * Runs every file_util unit test, aborting on the first failure.
 */
void run_all_file_util_tests()
{
  test_write_read_round_trip();
  test_read_tolerates_crlf_line_ending();
  test_read_drains_overlong_line();
  test_read_line_rejects_invalid_arguments();
  test_write_line_rejects_invalid_arguments();
  test_build_file_path_joins_correctly();
  test_build_file_path_null_inputs_are_safe();
  test_build_file_path_overflow_produces_empty();
}

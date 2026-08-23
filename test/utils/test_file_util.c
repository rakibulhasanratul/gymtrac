#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "../../src/settings.h"
#include "../../src/utils/file_util.h"
#include "test_file_util.h"

#define TEST_FILE_PATH DATA_DIRECTORY "/tmp_test_file_util.dat"
#define TEST_LINES_FILE_PATH DATA_DIRECTORY "/tmp_test_lines_util.dat"

/**
 * Opens the test file for writing and returns the open file.
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
void test_write_read_round_trip()
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
void test_read_tolerates_crlf_line_ending()
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
void test_read_drains_overlong_line()
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
void test_read_line_rejects_invalid_arguments()
{
  char buffer[64];

  assert(read_line_from_file(NULL, buffer, sizeof(buffer)) == false);
  assert(read_line_from_file(stdin, NULL, sizeof(buffer)) == false);
  assert(read_line_from_file(stdin, buffer, 1) == false);
}

/**
 * Verifies that write_line_to_file rejects invalid arguments.
 */
void test_write_line_rejects_invalid_arguments()
{
  assert(write_line_to_file(NULL, "line") == false);
  assert(write_line_to_file(stdout, NULL) == false);
}

/**
 * Verifies that lines written with write_lines_to_file read back unchanged
 * with read_lines_from_file.
 */
void test_write_read_lines_round_trip()
{
  char storage[4][64];
  char *maps[4];

  const char *lines[] = {"alpha", "beta"};
  assert(write_lines_to_file(TEST_LINES_FILE_PATH, lines, 2) == true);

  for (int i = 0; i < 4; i++) maps[i] = storage[i];

  assert(read_lines_from_file(TEST_LINES_FILE_PATH, maps, 4, 64) == 2);
  assert(strcmp(storage[0], "alpha") == 0);
  assert(strcmp(storage[1], "beta") == 0);
}

/**
 * Verifies that read_lines_from_file skips empty lines in the file.
 */
void test_read_lines_skip_empty_lines()
{
  char storage[4][64];
  char *maps[4];

  FILE *file = fopen(TEST_LINES_FILE_PATH, "w");
  assert(file != NULL);
  assert(write_line_to_file(file, "alpha"));
  assert(write_line_to_file(file, ""));
  assert(write_line_to_file(file, "beta"));
  fclose(file);

  for (int i = 0; i < 4; i++) maps[i] = storage[i];

  assert(read_lines_from_file(TEST_LINES_FILE_PATH, maps, 4, 64) == 2);
  assert(strcmp(storage[0], "alpha") == 0);
  assert(strcmp(storage[1], "beta") == 0);
}

/**
 * Verifies that write_lines_to_file replaces existing file content.
 */
void test_write_lines_overwrite_existing_content()
{
  char storage[4][64];
  char *maps[4];

  const char *first[] = {"one", "two", "three"};
  assert(write_lines_to_file(TEST_LINES_FILE_PATH, first, 3) == true);

  const char *second[] = {"only"};
  assert(write_lines_to_file(TEST_LINES_FILE_PATH, second, 1) == true);

  for (int i = 0; i < 4; i++) maps[i] = storage[i];

  assert(read_lines_from_file(TEST_LINES_FILE_PATH, maps, 4, 64) == 1);
  assert(strcmp(storage[0], "only") == 0);
}

/**
 * Verifies that read_lines_from_file stops at max_lines.
 */
void test_read_lines_respects_max_lines()
{
  char storage[4][64];
  char *maps[4];

  const char *lines[] = {"one", "two", "three"};
  assert(write_lines_to_file(TEST_LINES_FILE_PATH, lines, 3) == true);

  for (int i = 0; i < 4; i++) maps[i] = storage[i];

  assert(read_lines_from_file(TEST_LINES_FILE_PATH, maps, 2, 64) == 2);
  assert(strcmp(storage[0], "one") == 0);
  assert(strcmp(storage[1], "two") == 0);
}

/**
 * Verifies that reading a missing file returns zero lines.
 */
void test_read_lines_missing_file_returns_zero()
{
  char storage[4][64];
  char *maps[4];

  remove(TEST_LINES_FILE_PATH);

  for (int i = 0; i < 4; i++) maps[i] = storage[i];

  assert(read_lines_from_file(TEST_LINES_FILE_PATH, maps, 4, 64) == 0);
}

/**
 * Verifies that write_lines_to_file rejects invalid arguments.
 */
void test_write_lines_rejects_invalid_arguments()
{
  const char *lines[] = {"line"};

  assert(write_lines_to_file(NULL, lines, 1) == false);
  assert(write_lines_to_file(TEST_LINES_FILE_PATH, NULL, 1) == false);
  assert(write_lines_to_file(TEST_LINES_FILE_PATH, lines, -1) == false);

  const char *null_element[] = {NULL};
  assert(write_lines_to_file(TEST_LINES_FILE_PATH, null_element, 1) == false);
}

/**
 * Verifies that read_lines_from_file rejects invalid arguments.
 */
void test_read_lines_rejects_invalid_arguments()
{
  char storage[4][64];
  char *maps[4];

  for (int i = 0; i < 4; i++) maps[i] = storage[i];

  assert(read_lines_from_file(NULL, maps, 4, 64) == 0);
  assert(read_lines_from_file(TEST_LINES_FILE_PATH, NULL, 4, 64) == 0);
  assert(read_lines_from_file(TEST_LINES_FILE_PATH, maps, 0, 64) == 0);
  assert(read_lines_from_file(TEST_LINES_FILE_PATH, maps, -1, 64) == 0);
  assert(read_lines_from_file(TEST_LINES_FILE_PATH, maps, 4, 1) == 0);
}

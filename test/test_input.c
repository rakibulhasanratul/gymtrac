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
static void redirect_stdin_with_content(const char *content) {
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
static void test_read_string_plain_lines(void) {
    char buffer[64];

    redirect_stdin_with_content("hello world\nsecond\n");
    assert(input_string(buffer, sizeof buffer) == true);
    assert(strcmp(buffer, "hello world") == 0);
    assert(input_string(buffer, sizeof buffer) == true);
    assert(strcmp(buffer, "second") == 0);
    assert(input_string(buffer, sizeof buffer) == false);
}

/**
 * Verifies that input_string tolerates CRLF line endings.
 */
static void test_read_string_crlf(void) {
    char buffer[64];

    redirect_stdin_with_content("foo\r\nbar\n");
    assert(input_string(buffer, sizeof buffer) == true);
    assert(strcmp(buffer, "foo") == 0);
    assert(input_string(buffer, sizeof buffer) == true);
    assert(strcmp(buffer, "bar") == 0);
}

/**
 * Verifies that an over-long line is capped and its remainder drained.
 */
static void test_read_string_caps_and_drains(void) {
    char buffer[8];

    redirect_stdin_with_content("toolonginput\nnext\n");
    assert(input_string(buffer, sizeof buffer) == true);
    assert(strcmp(buffer, "toolong") == 0);
    assert(input_string(buffer, sizeof buffer) == true);
    assert(strcmp(buffer, "next") == 0);
}

/**
 * Verifies that an empty line reads back as an empty string.
 */
static void test_read_string_empty_line(void) {
    char buffer[64];

    redirect_stdin_with_content("\n");
    assert(input_string(buffer, sizeof buffer) == true);
    assert(strcmp(buffer, "") == 0);
}

/**
 * Verifies that input_string rejects invalid arguments.
 */
static void test_read_string_invalid_arguments(void) {
    char buffer[64];

    redirect_stdin_with_content("anything\n");
    assert(input_string(NULL, sizeof buffer) == false);
    assert(input_string(buffer, 1) == false);
}

/**
 * Verifies that input_unsigned_int accepts digits up to unsigned int max.
 */
static void test_read_unsigned_valid(void) {
    unsigned int value;

    redirect_stdin_with_content("42\n0\n4294967295\n");
    assert(input_unsigned_int(&value) == true);
    assert(value == 42u);
    assert(input_unsigned_int(&value) == true);
    assert(value == 0u);
    assert(input_unsigned_int(&value) == true);
    assert(value == UINT_MAX);
    assert(input_unsigned_int(&value) == false);
}

/**
 * Verifies that input_unsigned_int rejects signs, non-digits, and overflow.
 */
static void test_read_unsigned_rejects_bad(void) {
    unsigned int value;

    redirect_stdin_with_content("-5\n");
    assert(input_unsigned_int(&value) == false);

    redirect_stdin_with_content("12abc\n");
    assert(input_unsigned_int(&value) == false);

    redirect_stdin_with_content("4294967296\n");
    assert(input_unsigned_int(&value) == false);

    redirect_stdin_with_content("");
    assert(input_unsigned_int(&value) == false);

    assert(input_unsigned_int(NULL) == false);
}

/**
 * Verifies that the remainder of a number line is drained for later reads.
 */
static void test_read_unsigned_drains_line(void) {
    unsigned int value;
    char buffer[64];

    redirect_stdin_with_content("42 junk\nnext\n");
    assert(input_unsigned_int(&value) == true);
    assert(value == 42u);
    assert(input_string(buffer, sizeof buffer) == true);
    assert(strcmp(buffer, "next") == 0);
}

/**
 * Verifies that input_unsigned_long accepts digits up to the max value.
 */
static void test_read_unsigned_long_valid(void) {
    unsigned long int value;

    redirect_stdin_with_content("123\n18446744073709551615\n");
    assert(input_unsigned_long(&value) == true);
    assert(value == 123ul);
    assert(input_unsigned_long(&value) == true);
    assert(value == ULONG_MAX);
    assert(input_unsigned_long(&value) == false);
}

/**
 * Verifies that input_unsigned_long rejects signs, non-digits, overflow.
 */
static void test_read_unsigned_long_rejects_bad(void) {
    unsigned long int value;

    redirect_stdin_with_content("-7\n");
    assert(input_unsigned_long(&value) == false);

    redirect_stdin_with_content("xyz\n");
    assert(input_unsigned_long(&value) == false);

    redirect_stdin_with_content("18446744073709551616\n");
    assert(input_unsigned_long(&value) == false);

    redirect_stdin_with_content("");
    assert(input_unsigned_long(&value) == false);

    assert(input_unsigned_long(NULL) == false);
}

/**
 * Runs every input unit test, aborting on the first failure.
 */
void run_input_tests(void) {
    test_read_string_plain_lines();
    test_read_string_crlf();
    test_read_string_caps_and_drains();
    test_read_string_empty_line();
    test_read_string_invalid_arguments();
    test_read_unsigned_valid();
    test_read_unsigned_rejects_bad();
    test_read_unsigned_drains_line();
    test_read_unsigned_long_valid();
    test_read_unsigned_long_rejects_bad();
}

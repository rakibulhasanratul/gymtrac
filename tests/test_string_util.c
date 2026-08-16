#include <assert.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "../src/utils/string_util.h"
#include "test_string_util.h"

/**
 * Verifies that string_trim removes leading and trailing whitespace.
 */
static void test_trim(void) {
    char buffer[64];
    char *result;

    strcpy(buffer, "  hello world  \t\n");
    result = string_trim(buffer);
    assert(result == buffer + 2);
    assert(strcmp(result, "hello world") == 0);

    strcpy(buffer, "\t\n  ");
    result = string_trim(buffer);
    assert(strcmp(result, "") == 0);

    strcpy(buffer, "no-space");
    result = string_trim(buffer);
    assert(strcmp(result, "no-space") == 0);

    assert(string_trim(NULL) == NULL);
}

/**
 * Verifies that string_split captures fields in place on a delimiter.
 */
static void test_split(void) {
    char buffer[64];
    char *parts[8];
    int part_count;

    strcpy(buffer, "alpha|beta|gamma");
    part_count = string_split(buffer, '|', parts, 8);
    assert(part_count == 3);
    assert(strcmp(parts[0], "alpha") == 0);
    assert(strcmp(parts[1], "beta") == 0);
    assert(strcmp(parts[2], "gamma") == 0);

    strcpy(buffer, "a||c");
    part_count = string_split(buffer, '|', parts, 8);
    assert(part_count == 3);
    assert(strcmp(parts[0], "a") == 0);
    assert(strcmp(parts[1], "") == 0);
    assert(strcmp(parts[2], "c") == 0);

    strcpy(buffer, "a|b|c|d");
    part_count = string_split(buffer, '|', parts, 2);
    assert(part_count == 2);
    assert(strcmp(parts[0], "a") == 0);
    assert(strcmp(parts[1], "b") == 0);

    strcpy(buffer, "solo");
    part_count = string_split(buffer, '|', parts, 8);
    assert(part_count == 1);
    assert(strcmp(parts[0], "solo") == 0);

    strcpy(buffer, "trailing|");
    part_count = string_split(buffer, '|', parts, 8);
    assert(part_count == 2);
    assert(strcmp(parts[0], "trailing") == 0);
    assert(strcmp(parts[1], "") == 0);

    strcpy(buffer, "salt:hash");
    part_count = string_split(buffer, ':', parts, 8);
    assert(part_count == 2);
    assert(strcmp(parts[0], "salt") == 0);
    assert(strcmp(parts[1], "hash") == 0);

    assert(string_split(NULL, '|', parts, 8) == 0);
    assert(string_split(buffer, '|', NULL, 8) == 0);
}

/**
 * Verifies that string_parse_unsigned accepts digits and rejects everything else.
 */
static void test_parse_unsigned(void) {
    unsigned int value;

    assert(string_parse_unsigned("12345", &value) == true);
    assert(value == 12345u);

    assert(string_parse_unsigned("0", &value) == true);
    assert(value == 0u);

    assert(string_parse_unsigned("4294967295", &value) == true);
    assert(value == UINT_MAX);

    assert(string_parse_unsigned("4294967296", &value) == false);
    assert(string_parse_unsigned("", &value) == false);
    assert(string_parse_unsigned("12a", &value) == false);
    assert(string_parse_unsigned(" 12", &value) == false);
    assert(string_parse_unsigned("+12", &value) == false);
    assert(string_parse_unsigned(NULL, &value) == false);
}

/**
 * Verifies that string_parse_unsigned_long accepts digits and rejects everything else.
 */
static void test_parse_unsigned_long(void) {
    unsigned long int value;

    assert(string_parse_unsigned_long("123", &value) == true);
    assert(value == 123ul);

    assert(string_parse_unsigned_long("18446744073709551615", &value) == true);
    assert(value == ULONG_MAX);

    assert(string_parse_unsigned_long("18446744073709551616", &value) == false);
    assert(string_parse_unsigned_long("abc", &value) == false);
    assert(string_parse_unsigned_long("", &value) == false);
    assert(string_parse_unsigned_long(NULL, &value) == false);
}

/**
 * Verifies that string_to_lower and string_to_upper convert letters in place.
 */
static void test_case_conversion(void) {
    char buffer[64];
    char *result;

    strcpy(buffer, "HeLLo WoRLD 123");
    result = string_to_lower(buffer);
    assert(result == buffer);
    assert(strcmp(buffer, "hello world 123") == 0);

    strcpy(buffer, "HeLLo WoRLD 123");
    result = string_to_upper(buffer);
    assert(result == buffer);
    assert(strcmp(buffer, "HELLO WORLD 123") == 0);

    assert(string_to_lower(NULL) == NULL);
    assert(string_to_upper(NULL) == NULL);
}

/**
 * Runs every string_util unit test, aborting on the first failure.
 */
void run_string_util_tests(void) {
    test_trim();
    test_split();
    test_parse_unsigned();
    test_parse_unsigned_long();
    test_case_conversion();
}

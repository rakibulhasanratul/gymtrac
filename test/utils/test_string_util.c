#include <assert.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "../../src/settings.h"
#include "../../src/utils/string_util.h"
#include "test_string_util.h"

/**
 * Verifies that trim copies text stripped of leading and trailing
 * whitespace.
 */
void test_trim_strips_whitespace()
{
  char input[64];
  char result[64];

  strcpy(input, "  hello world  \t\n");
  trim(result, sizeof(result), input);
  assert(strcmp(result, "hello world") == 0);
  assert(strcmp(input, "  hello world  \t\n") == 0);

  strcpy(input, "\t\n  ");
  trim(result, sizeof(result), input);
  assert(strcmp(result, "") == 0);

  strcpy(input, "no-space");
  trim(result, sizeof(result), input);
  assert(strcmp(result, "no-space") == 0);

  strcpy(input, "hello world");
  trim(result, 6, input);
  assert(strcmp(result, "hello") == 0);

  trim(NULL, sizeof(result), input);
  trim(result, sizeof(result), NULL);
}

/**
 * Verifies that split copies each field into its own part buffer.
 */
void test_split_copies_fields()
{
  char buffer[64];
  char fields[8][64];
  char *parts[8];
  int part_index;
  int part_count;

  for (part_index = 0; part_index < 8; part_index++) parts[part_index] = fields[part_index];

  strcpy(buffer, "alpha|beta|gamma");
  part_count = split(buffer, '|', parts, 8, 64);
  assert(part_count == 3);
  assert(strcmp(parts[0], "alpha") == 0);
  assert(strcmp(parts[1], "beta") == 0);
  assert(strcmp(parts[2], "gamma") == 0);
  assert(strcmp(buffer, "alpha|beta|gamma") == 0);

  strcpy(buffer, "a||c");
  part_count = split(buffer, '|', parts, 8, 64);
  assert(part_count == 3);
  assert(strcmp(parts[0], "a") == 0);
  assert(strcmp(parts[1], "") == 0);
  assert(strcmp(parts[2], "c") == 0);

  strcpy(buffer, "a|b|c|d");
  part_count = split(buffer, '|', parts, 2, 64);
  assert(part_count == 2);
  assert(strcmp(parts[0], "a") == 0);
  assert(strcmp(parts[1], "b") == 0);

  strcpy(buffer, "solo");
  part_count = split(buffer, '|', parts, 8, 64);
  assert(part_count == 1);
  assert(strcmp(parts[0], "solo") == 0);

  strcpy(buffer, "trailing|");
  part_count = split(buffer, '|', parts, 8, 64);
  assert(part_count == 2);
  assert(strcmp(parts[0], "trailing") == 0);
  assert(strcmp(parts[1], "") == 0);

  strcpy(buffer, "salt:hash");
  part_count = split(buffer, ':', parts, 8, 64);
  assert(part_count == 2);
  assert(strcmp(parts[0], "salt") == 0);
  assert(strcmp(parts[1], "hash") == 0);

  strcpy(buffer, "abcdef|g");
  part_count = split(buffer, '|', parts, 8, 4);
  assert(part_count == 2);
  assert(strcmp(parts[0], "abc") == 0);
  assert(strcmp(parts[1], "g") == 0);

  assert(split(NULL, '|', parts, 8, 64) == 0);
  assert(split(buffer, '|', NULL, 8, 64) == 0);
  assert(split(buffer, '|', parts, 0, 64) == 0);
  assert(split(buffer, '|', parts, 8, 1) == 0);
}

/**
 * Verifies that string_to_unsigned_int accepts digits and rejects
 * everything else.
 */
void test_string_to_unsigned_int_converts_digits()
{
  assert(string_to_unsigned_int("12345") == 12345u);
  assert(string_to_unsigned_int("0") == 0u);
  assert(string_to_unsigned_int("4294967295") == UINT_MAX);

  assert(string_to_unsigned_int("4294967296") == 0);
  assert(string_to_unsigned_int("") == 0);
  assert(string_to_unsigned_int("12a") == 0);
  assert(string_to_unsigned_int(" 12") == 0);
  assert(string_to_unsigned_int("+12") == 0);
  assert(string_to_unsigned_int(NULL) == 0);
}

/**
 * Verifies that string_to_unsigned_long_int accepts digits and rejects
 * everything else.
 */
void test_string_to_unsigned_long_int_converts_digits()
{
  // Built at runtime because unsigned long width differs per platform.
  char maximum[32];
  char overflow[32];

  sprintf(maximum, "%lu", ULONG_MAX);
  sprintf(overflow, "%lu0", ULONG_MAX);

  assert(string_to_unsigned_long_int("123") == 123ul);
  assert(string_to_unsigned_long_int(maximum) == ULONG_MAX);

  assert(string_to_unsigned_long_int(overflow) == 0);
  assert(string_to_unsigned_long_int("abc") == 0);
  assert(string_to_unsigned_long_int("") == 0);
  assert(string_to_unsigned_long_int(NULL) == 0);
}

/**
 * Verifies that to_lowercase and to_uppercase convert letters in place.
 */
void test_lowercase_and_uppercase_convert_letters()
{
  char buffer[64];
  char *result;

  strcpy(buffer, "HeLLo WoRLD 123");
  result = to_lowercase(buffer);
  assert(result == buffer);
  assert(strcmp(buffer, "hello world 123") == 0);

  strcpy(buffer, "HeLLo WoRLD 123");
  result = to_uppercase(buffer);
  assert(result == buffer);
  assert(strcmp(buffer, "HELLO WORLD 123") == 0);

  assert(to_lowercase(NULL) == NULL);
  assert(to_uppercase(NULL) == NULL);
}

/**
 * Verifies that sanitize_field strips control characters and delimiters.
 */
void test_sanitize_field_strips_control_chars()
{
  char buffer[128];

  strcpy(buffer, "bad\t|\nchar\x01s\r");
  assert(sanitize_field(buffer) == buffer);
  assert(strcmp(buffer, "badchars") == 0);

  strcpy(buffer, "clean|text");
  assert(sanitize_field(buffer) == buffer);
  assert(strcmp(buffer, "cleantext") == 0);

  strcpy(buffer, "no change needed");
  assert(sanitize_field(buffer) == buffer);
  assert(strcmp(buffer, "no change needed") == 0);

  strcpy(buffer, "||||");
  assert(sanitize_field(buffer) == buffer);
  assert(strcmp(buffer, "") == 0);

  assert(sanitize_field(NULL) == NULL);
}

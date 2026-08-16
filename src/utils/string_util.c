#include <ctype.h>
#include <limits.h>
#include <string.h>

#include "string_util.h"

void string_trim(char destination[], int destination_capacity, const char text[]) {
  int start_index;
  int end_index;
  int write_index;

  if (destination == NULL || text == NULL || destination_capacity < 2) {
    return;
  }

  // Skip past leading whitespace.
  start_index = 0;
  while (text[start_index] != '\0' && isspace((unsigned char)text[start_index])) {
    start_index++;
  }

  // Scan to the end of the string.
  end_index = start_index;
  while (text[end_index] != '\0') {
    end_index++;
  }

  // Back up over trailing whitespace.
  while (end_index > start_index && isspace((unsigned char)text[end_index - 1])) {
    end_index--;
  }

  // Copy the trimmed slice into destination, capped by the buffer size.
  write_index = 0;
  while (start_index < end_index && write_index < destination_capacity - 1) {
    destination[write_index] = text[start_index];
    write_index++;
    start_index++;
  }
  destination[write_index] = '\0';
}

int string_split(const char text[], char delimiter, char *parts[], int part_capacity, int field_capacity) {
  int part_count;
  int cursor_index;
  int field_index;

  if (text == NULL || parts == NULL || part_capacity < 1 || field_capacity < 2) {
    return 0;
  }

  part_count = 0;
  cursor_index = 0;
  while (part_count < part_capacity) {
    // Copy the current field into the next part.
    field_index = 0;
    while (text[cursor_index] != '\0' && text[cursor_index] != delimiter) {
      // Write only the characters that fit in the buffer.
      if (field_index < field_capacity - 1) {
        parts[part_count][field_index] = text[cursor_index];
        field_index++;
      }
      cursor_index++;
    }
    parts[part_count][field_index] = '\0';
    part_count++;
    // Stop once the string is exhausted.
    if (text[cursor_index] == '\0') {
      break;
    }
    // Skip the delimiter to reach the next field.
    cursor_index++;
  }

  return part_count;
}

bool string_parse_unsigned(const char text[], unsigned int *value) {
  unsigned int accumulated;
  int index;

  if (text == NULL || value == NULL || text[0] == '\0') {
    return false;
  }

  accumulated = 0;
  for (index = 0; text[index] != '\0'; index++) {
    unsigned int digit;
    // Reject any non-digit character.
    if (text[index] < '0' || text[index] > '9') {
      return false;
    }
    digit = (unsigned int)(text[index] - '0');
    // Reject when the next digit would overflow the result.
    if (accumulated > (UINT_MAX - digit) / 10u) {
      return false;
    }
    accumulated = accumulated * 10u + digit;
  }

  *value = accumulated;
  return true;
}

bool string_parse_unsigned_long(const char text[], unsigned long int *value) {
  unsigned long int accumulated;
  int index;

  if (text == NULL || value == NULL || text[0] == '\0') {
    return false;
  }

  accumulated = 0;
  for (index = 0; text[index] != '\0'; index++) {
    unsigned long int digit;
    // Reject any non-digit character.
    if (text[index] < '0' || text[index] > '9') {
      return false;
    }
    digit = (unsigned long int)(text[index] - '0');
    // Reject when the next digit would overflow the result.
    if (accumulated > (ULONG_MAX - digit) / 10ul) {
      return false;
    }
    accumulated = accumulated * 10ul + digit;
  }

  *value = accumulated;
  return true;
}

char *string_to_lower(char text[]) {
  int index;

  if (text == NULL) {
    return NULL;
  }

  // Fold every letter to lowercase in place.
  for (index = 0; text[index] != '\0'; index++) {
    text[index] = (char)tolower((unsigned char)text[index]);
  }

  return text;
}

char *string_to_upper(char text[]) {
  int index;

  if (text == NULL) {
    return NULL;
  }

  // Fold every letter to uppercase in place.
  for (index = 0; text[index] != '\0'; index++) {
    text[index] = (char)toupper((unsigned char)text[index]);
  }

  return text;
}

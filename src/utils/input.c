#include <stdio.h>
#include <string.h>

#include "input.h"

// Discards every character up to the end of the current input line.
static void input_discard_line(void) {
  int ch;
  while ((ch = getchar()) != '\n' && ch != EOF) {
    // Swallow the character and keep scanning for the newline.
  }
}

bool input_string(char buffer[], int buffer_capacity) {
  int length;

  if (buffer == NULL || buffer_capacity < 2) {
    return false;
  }

  if (fgets(buffer, buffer_capacity, stdin) == NULL) {
    return false;
  }

  length = (int)strlen(buffer);
  // Strip the trailing newline and any preceding carriage return.
  if (length > 0 && buffer[length - 1] == '\n') {
    buffer[length - 1] = '\0';
    if (length > 1 && buffer[length - 2] == '\r') {
      buffer[length - 2] = '\0';
    }
    return true;
  }

  // The line overflowed the buffer; drain it so the next read stays clean.
  input_discard_line();
  return true;
}

bool input_integer(int *value) {
  int matched;

  if (value == NULL) {
    return false;
  }

  matched = scanf("%d", value);
  if (matched != 1) {
    input_discard_line();
    return false;
  }
  // Consume the rest of the line so the next read starts on a fresh line.
  input_discard_line();
  return true;
}

bool input_positive_int(int *value) {
  if (!input_integer(value)) {
    return false;
  }
  // Reject zero and negative numbers so only a positive amount is kept.
  if (*value <= 0) {
    return false;
  }
  return true;
}

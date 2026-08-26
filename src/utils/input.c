#include <stdio.h>
#include <string.h>

#include "input.h"

// Discards every character up to the end of the current input line.
static inline void discard_remaining_input()
{
  int ch;
  while ((ch = getchar()) != '\n' && ch != EOF);
}

bool input_string(char buffer[], int buffer_capacity)
{
  if (buffer == NULL || buffer_capacity < 2) return false;

  if (fgets(buffer, buffer_capacity, stdin) == NULL) return false;

  int length = (int)strlen(buffer);
  // Strip the trailing newline and any preceding carriage return.
  if (length > 0 && buffer[length - 1] == '\n')
  {
    buffer[length - 1] = '\0';
    if (length > 1 && buffer[length - 2] == '\r') buffer[length - 2] = '\0';
    return true;
  }

  // The line overflowed the buffer; drain it so the next read stays clean.
  discard_remaining_input();
  return true;
}

bool input_integer(int *value)
{
  if (value == NULL) return false;

  int matched = scanf("%d", value);
  if (matched != 1)
  {
    discard_remaining_input();
    return false;
  }
  // Consume the rest of the line so the next read starts on a fresh line.
  discard_remaining_input();
  return true;
}

bool input_positive_int(int *value)
{
  if (!input_integer(value)) return false;
  // Reject anything but positives.
  if (*value <= 0) return false;
  return true;
}

#include <stdio.h>
#include <string.h>

#include "input.h"

// Discards every character up to the end of the current input line.
static inline void discard_remaining_input()
{
  int ch;
  while ((ch = getchar()) != '\n' && ch != EOF);
}

bool input_string(char *destination, int destination_capacity)
{
  if (destination == NULL || destination_capacity < 2) return false;

  if (fgets(destination, destination_capacity, stdin) == NULL) return false;

  int length = (int)strlen(destination);
  // Strip the trailing newline and any preceding carriage return.
  if (length > 0 && destination[length - 1] == '\n')
  {
    destination[length - 1] = '\0';
    if (length > 1 && destination[length - 2] == '\r') destination[length - 2] = '\0';
    return true;
  }

  // The line overflowed the buffer; drain it so the next read stays clean.
  discard_remaining_input();
  return true;
}

bool input_integer(int *destination)
{
  if (destination == NULL) return false;

  int matched = scanf("%d", destination);
  if (matched != 1)
  {
    discard_remaining_input();
    return false;
  }
  // Consume the rest of the line so the next read starts on a fresh line.
  discard_remaining_input();
  return true;
}

bool input_positive_int(int *destination)
{
  if (!input_integer(destination)) return false;
  // Reject anything but positives.
  if (*destination <= 0) return false;
  return true;
}

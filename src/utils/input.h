#ifndef GYMTRAC_INPUT_H
#define GYMTRAC_INPUT_H

#include <stdbool.h>

/**
 * Reads one line from standard input into buffer, capped at buffer_capacity.
 *
 * The trailing newline is stripped. When the typed line is longer than the
 * buffer, the overflow is discarded and the truncated value is kept, so the
 * next read never sees the remainder of the previous line.
 *
 * @param buffer receives the line without its trailing newline
 * @param buffer_capacity the number of characters buffer can hold
 * @return true when a line was read, false on invalid input or end of input
 */
bool input_string(char *buffer, int buffer_capacity);

/**
 * Reads a whole number from standard input into value.
 *
 * The rest of the input line is discarded after the number, so a later read
 * never sees stray characters. Non-numeric input is rejected.
 *
 * @param value receives the parsed number on success
 * @return true when a valid number was read, false otherwise
 */
bool input_integer(int *value);

/**
 * Reads a strictly positive whole number from standard input into value.
 *
 * The rest of the input line is discarded after the number, so a later read
 * never sees stray characters. Non-numeric input, zero, and negative numbers
 * are rejected.
 *
 * @param value receives the parsed number on success
 * @return true when a valid positive number was read, false otherwise
 */
bool input_positive_int(int *value);

#endif

#ifndef GYMTRAC_INPUT_H
#define GYMTRAC_INPUT_H

#include <stdbool.h>

/**
 * Reads one line from standard input into destination, capped at
 * destination_capacity.
 *
 * Strips the trailing newline. A longer line is truncated and its remainder
 * discarded, so the next read starts fresh.
 *
 * @param destination receives the line without its trailing newline
 * @param destination_capacity the number of characters destination can hold
 * @return true when a line was read, false on invalid input or end of input
 */
bool input_string(char *destination, int destination_capacity);

/**
 * Reads a whole number from standard input into destination.
 *
 * Discards the rest of the input line after the number. Rejects non-numeric
 * input.
 *
 * @param destination receives the parsed number on success
 * @return true when a valid number was read, false otherwise
 */
bool input_integer(int *destination);

/**
 * Reads a strictly positive whole number from standard input into destination.
 *
 * Discards the rest of the input line after the number. Rejects non-numeric
 * input, zero, and negatives.
 *
 * @param destination receives the parsed number on success
 * @return true when a valid positive number was read, false otherwise
 */
bool input_positive_int(int *destination);

#endif

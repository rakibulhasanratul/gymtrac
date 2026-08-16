#ifndef GYMTRAC_FILE_UTIL_H
#define GYMTRAC_FILE_UTIL_H

#include <stdbool.h>
#include <stdio.h>

// Delimiter separating fields inside a persisted record line.
#define FIELD_DELIMITER '|'

/**
 * Reads the next line from file into buffer, stripping the trailing newline.
 *
 * When a line is longer than the buffer, its remainder is discarded and false
 * is returned so an over-long line is never parsed as multiple records.
 *
 * @param file the open file to read from
 * @param buffer receives the line without its trailing newline
 * @param buffer_capacity the number of characters buffer can hold
 * @return true when a line was read, false on invalid input or end of file
 */
bool file_read_line(FILE *file, char *buffer, int buffer_capacity);

/**
 * Writes line to file followed by a newline.
 *
 * @param file the open file to write to
 * @param line the record line to write
 * @return true when the line was written, false on invalid input or error
 */
bool file_write_line(FILE *file, const char *line);

/**
 * Removes control characters and the field delimiter from text in place.
 *
 * @param text the field value to clean
 * @return text, or NULL when text is NULL
 */
char *file_sanitize_field(char *text);

#endif

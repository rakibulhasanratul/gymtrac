#ifndef GYMTRAC_FILE_UTIL_H
#define GYMTRAC_FILE_UTIL_H

#include <stdbool.h>
#include <stdio.h>

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
bool read_line_from_file(FILE *file, char buffer[], int buffer_capacity);

/**
 * Writes line to file followed by a newline.
 *
 * @param file the open file to write to
 * @param line the record line to write
 * @return true when the line was written, false on invalid input or error
 */
bool write_line_to_file(FILE *file, const char line[]);

/**
 * Builds a full file path by joining the data directory with a filename.
 *
 * @param filename the bare file name (e.g. "branches.txt")
 * @param destination receives "DEFAULT_DATA_DIRECTORY/filename"
 * @param destination_capacity the number of characters destination can hold
 */
void build_file_path(const char *filename, char *destination, int destination_capacity);

#endif

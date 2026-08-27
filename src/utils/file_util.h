#ifndef GYMTRAC_FILE_UTIL_H
#define GYMTRAC_FILE_UTIL_H

#include <stdbool.h>
#include <stdio.h>

/**
 * Reads the next line from file into destination, stripping the trailing
 * newline.
 *
 * An over-long line drains its remainder and returns false instead of
 * parsing as multiple records.
 *
 * @param file the open file to read from
 * @param destination receives the line without its trailing newline
 * @param destination_capacity the number of characters destination can hold
 * @return true when a line was read, false on invalid input or end of file
 */
bool read_line_from_file(FILE *file, char *destination, int destination_capacity);

/**
 * Writes line to file followed by a newline.
 *
 * @param file the open file to write to
 * @param line the record line to write
 * @return true when the line was written, false on invalid input or error
 */
bool write_line_to_file(FILE *file, const char line[]);

/**
 * Reads every line of a file into destination, skipping empty lines.
 *
 * Each row must hold at least line_capacity characters. Over-long lines are
 * drained and skipped, keeping corrupted records out of the loaded data.
 *
 * @param file_path the path of the file to read
 * @param destination receives one non-empty line per row;
 *                    rows are filled through char pointers
 * @param max_lines the maximum number of lines to read
 * @param line_capacity the capacity of each row in destination
 * @return the number of lines read, 0 on invalid arguments or when the
 *         file cannot be opened
 */
int read_lines_from_file(const char file_path[], char *destination[], int max_lines, int line_capacity);

/**
 * Writes each element of lines as one line into the file.
 *
 * The file is opened with "w" so any existing content is replaced; a count
 * of zero leaves the file empty.
 *
 * @param file_path the path of the file to write
 * @param lines the lines to write; elements are read through char pointers
 * @param line_count the number of lines to write
 * @return true when every line was written, false on invalid input or error
 */
bool write_lines_to_file(const char file_path[], const char *lines[], int line_count);

#endif

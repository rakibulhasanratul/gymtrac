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
 * Reads every line of a file into destination, skipping empty lines.
 *
 * Each row of lines_destination must hold at least line_capacity characters.
 * Over-long lines are drained and skipped so a corrupted record never leaks
 * into the loaded data.
 *
 * @param file_path the path of the file to read
 * @param lines_destination receives one non-empty line per row;
 *                          rows are filled through char pointers
 * @param max_lines the maximum number of lines to read
 * @param line_capacity the capacity of each row in lines_destination
 * @return the number of lines read, 0 on invalid arguments or when the
 *         file cannot be opened
 */
int read_lines_from_file(const char file_path[], char *lines_destination[], int max_lines, int line_capacity);

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

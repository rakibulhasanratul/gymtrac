#include <stdio.h>
#include <string.h>

#include "file_util.h"
#include "string_util.h"

bool read_line_from_file(FILE *file, char *destination, int destination_capacity)
{
  if (file == NULL || destination == NULL || destination_capacity < 2) return false;

  if (fgets(destination, destination_capacity, file) == NULL) return false;

  int length = (int)strlen(destination);
  // Strip the trailing newline and any preceding carriage return.
  if (length > 0 && destination[length - 1] == '\n')
  {
    destination[length - 1] = '\0';
    if (length > 1 && destination[length - 2] == '\r') destination[length - 2] = '\0';
    return true;
  }

  // Accept a final line that ends without a newline at end of file.
  if (feof(file)) return true;

  // Drain an over-long line's remainder so reads never split it.
  while (!feof(file) && !ferror(file))
  {
    if (fgetc(file) == '\n') break;
  }
  return false;
}

bool write_line_to_file(FILE *file, const char line[])
{
  if (file == NULL || line == NULL) return false;
  // Write the record followed by its terminating newline.
  if (fputs(line, file) == EOF) return false;
  if (fputc('\n', file) == EOF) return false;
  return true;
}

int read_lines_from_file(const char file_path[], char *destination[], int max_lines, int line_capacity)
{
  if (file_path == NULL || destination == NULL || max_lines < 1 || line_capacity < 2) return 0;

  FILE *file = fopen(file_path, "r");
  if (file == NULL) return 0;

  int line_count = 0;
  while (line_count < max_lines && read_line_from_file(file, destination[line_count], line_capacity))
  {
    // Empty lines carry no record and stay invisible to callers.
    if (!is_blank_string(destination[line_count])) line_count++;
  }

  fclose(file);
  return line_count;
}

bool write_lines_to_file(const char file_path[], const char *lines[], int line_count)
{
  if (file_path == NULL || lines == NULL || line_count < 0) return false;

  FILE *file = fopen(file_path, "w");
  if (file == NULL) return false;

  for (int i = 0; i < line_count; i++)
  {
    if (lines[i] == NULL || !write_line_to_file(file, lines[i]))
    {
      fclose(file);
      return false;
    }
  }

  fclose(file);
  return true;
}

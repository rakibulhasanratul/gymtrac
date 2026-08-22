#include <stdio.h>
#include <string.h>

#include "file_util.h"

bool read_line_from_file(FILE *file, char buffer[], int buffer_capacity)
{
  if (file == NULL || buffer == NULL || buffer_capacity < 2)
    return false;

  if (fgets(buffer, buffer_capacity, file) == NULL)
    return false;

  int length = (int)strlen(buffer);
  // Strip the trailing newline and any preceding carriage return.
  if (length > 0 && buffer[length - 1] == '\n')
  {
    buffer[length - 1] = '\0';
    if (length > 1 && buffer[length - 2] == '\r')
      buffer[length - 2] = '\0';
    return true;
  }

  // Accept a final line that ends without a newline at end of file.
  if (feof(file))
    return true;

  // Drain the remainder of an over-long line so it is never split across
  // reads.
  while (!feof(file) && !ferror(file))
  {
    if (fgetc(file) == '\n')
      break;
  }
  return false;
}

bool write_line_to_file(FILE *file, const char line[])
{
  if (file == NULL || line == NULL)
    return false;
  // Write the record followed by its terminating newline.
  if (fputs(line, file) == EOF)
    return false;
  if (fputc('\n', file) == EOF)
    return false;
  return true;
}

int read_lines_from_file(const char file_path[], char *lines_destination[], int max_lines, int line_capacity)
{
  if (file_path == NULL || lines_destination == NULL || max_lines < 1 || line_capacity < 2)
    return 0;

  FILE *file = fopen(file_path, "r");
  if (file == NULL)
    return 0;

  int line_count = 0;
  while (line_count < max_lines && read_line_from_file(file, lines_destination[line_count], line_capacity))
  {
    // Empty lines carry no record and stay invisible to callers.
    if (strlen(lines_destination[line_count]) > 0)
      line_count++;
  }

  fclose(file);
  return line_count;
}

bool write_lines_to_file(const char file_path[], const char *lines[], int line_count)
{
  if (file_path == NULL || lines == NULL || line_count < 0)
    return false;

  FILE *file = fopen(file_path, "w");
  if (file == NULL)
    return false;

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

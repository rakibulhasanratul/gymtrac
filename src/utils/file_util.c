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

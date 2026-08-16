#include <ctype.h>
#include <string.h>

#include "file_util.h"

bool file_read_line(FILE *file, char *buffer, int buffer_capacity) {
  int length;

  if (file == NULL || buffer == NULL || buffer_capacity < 2) {
    return false;
  }

  if (fgets(buffer, buffer_capacity, file) == NULL) {
    return false;
  }

  length = (int)strlen(buffer);
  // Strip the trailing newline and any preceding carriage return.
  if (length > 0 && buffer[length - 1] == '\n') {
    buffer[length - 1] = '\0';
    if (length > 1 && buffer[length - 2] == '\r') {
      buffer[length - 2] = '\0';
    }
    return true;
  }

  // Accept a final line that ends without a newline at end of file.
  if (feof(file)) {
    return true;
  }

  // Drain the remainder of an over-long line so it is never split across reads.
  while (!feof(file) && !ferror(file)) {
    if (fgetc(file) == '\n') {
      break;
    }
  }
  return false;
}

bool file_write_line(FILE *file, const char *line) {
  if (file == NULL || line == NULL) {
    return false;
  }
  // Write the record followed by its terminating newline.
  if (fputs(line, file) == EOF) {
    return false;
  }
  if (fputc('\n', file) == EOF) {
    return false;
  }
  return true;
}

char *file_sanitize_field(char *text) {
  char *write_cursor;
  char *read_cursor;

  if (text == NULL) {
    return NULL;
  }

  write_cursor = text;
  for (read_cursor = text; *read_cursor != '\0'; read_cursor++) {
    unsigned char ch = (unsigned char)*read_cursor;
    // Drop the field delimiter and any control character.
    if (ch == (unsigned char)FIELD_DELIMITER || iscntrl(ch)) {
      continue;
    }
    // Compact the kept characters toward the front.
    *write_cursor = *read_cursor;
    write_cursor++;
  }
  // Terminate the compacted string.
  *write_cursor = '\0';

  return text;
}

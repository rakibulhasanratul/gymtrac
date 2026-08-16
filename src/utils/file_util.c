#include <ctype.h>
#include <string.h>

#include "file_util.h"

bool file_read_line(FILE *file, char buffer[], int buffer_capacity) {
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

bool file_write_line(FILE *file, const char line[]) {
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

char *file_sanitize_field(char text[]) {
  int read_index;
  int write_index;

  if (text == NULL) {
    return NULL;
  }

  write_index = 0;
  for (read_index = 0; text[read_index] != '\0'; read_index++) {
    unsigned char ch = (unsigned char)text[read_index];
    // Drop the field delimiter and any control character.
    if (ch == (unsigned char)FIELD_DELIMITER || iscntrl(ch)) {
      continue;
    }
    // Compact the kept characters toward the front.
    text[write_index] = text[read_index];
    write_index++;
  }
  // Terminate the compacted string.
  text[write_index] = '\0';

  return text;
}

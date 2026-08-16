#include <stdio.h>
#include <string.h>

#include "input.h"
#include "string_util.h"

// Number token cap, wide enough for the largest unsigned long int.
#define INPUT_NUMBER_TOKEN_SIZE 32

// Discards every character up to the end of the current input line.
static void input_discard_line(void) {
    int ch;
    while ((ch = getchar()) != '\n' && ch != EOF) {
        // Swallow the character and keep scanning for the newline.
    }
}

bool input_string(char *buffer, int buffer_capacity) {
    int length;

    if (buffer == NULL || buffer_capacity < 2) {
        return false;
    }

    if (fgets(buffer, buffer_capacity, stdin) == NULL) {
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

    // The line overflowed the buffer; drain it so the next read stays clean.
    input_discard_line();
    return true;
}

bool input_unsigned_int(unsigned int *value) {
    char token[INPUT_NUMBER_TOKEN_SIZE];

    if (value == NULL) {
        return false;
    }
    // Bounded read so an over-long number can never overflow the token buffer.
    if (scanf("%31s", token) != 1) {
        input_discard_line();
        return false;
    }
    input_discard_line();
    // Parse and range-check the token; rejects signs and non-digits.
    return string_parse_unsigned(token, value);
}

bool input_unsigned_long(unsigned long int *value) {
    char token[INPUT_NUMBER_TOKEN_SIZE];

    if (value == NULL) {
        return false;
    }
    // Bounded read so an over-long number can never overflow the token buffer.
    if (scanf("%31s", token) != 1) {
        input_discard_line();
        return false;
    }
    input_discard_line();
    // Parse and range-check the token; rejects signs and non-digits.
    return string_parse_unsigned_long(token, value);
}

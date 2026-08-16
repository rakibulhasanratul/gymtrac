#include <ctype.h>
#include <limits.h>
#include <string.h>

#include "string_util.h"

char *string_trim(char *text) {
    char *start;
    char *end;

    if (text == NULL) {
        return NULL;
    }

    start = text;
    while (*start != '\0' && isspace((unsigned char)*start)) {
        start++;
    }

    end = start + strlen(start);
    while (end > start && isspace((unsigned char)end[-1])) {
        end--;
    }
    *end = '\0';

    return start;
}

int string_split(char *text, char delimiter, char **parts, int part_capacity) {
    char *cursor;
    int part_count;

    if (text == NULL || parts == NULL || part_capacity < 1) {
        return 0;
    }

    cursor = text;
    part_count = 0;
    while (part_count < part_capacity) {
        char *found;
        parts[part_count] = cursor;
        part_count++;
        found = strchr(cursor, delimiter);
        if (found == NULL) {
            break;
        }
        *found = '\0';
        cursor = found + 1;
    }

    return part_count;
}

bool string_parse_unsigned(const char *text, unsigned int *value) {
    unsigned int accumulated;
    const char *cursor;

    if (text == NULL || value == NULL || *text == '\0') {
        return false;
    }

    accumulated = 0;
    for (cursor = text; *cursor != '\0'; cursor++) {
        unsigned int digit;
        if (*cursor < '0' || *cursor > '9') {
            return false;
        }
        digit = (unsigned int)(*cursor - '0');
        if (accumulated > (UINT_MAX - digit) / 10u) {
            return false;
        }
        accumulated = accumulated * 10u + digit;
    }

    *value = accumulated;
    return true;
}

bool string_parse_unsigned_long(const char *text, unsigned long int *value) {
    unsigned long int accumulated;
    const char *cursor;

    if (text == NULL || value == NULL || *text == '\0') {
        return false;
    }

    accumulated = 0;
    for (cursor = text; *cursor != '\0'; cursor++) {
        unsigned long int digit;
        if (*cursor < '0' || *cursor > '9') {
            return false;
        }
        digit = (unsigned long int)(*cursor - '0');
        if (accumulated > (ULONG_MAX - digit) / 10ul) {
            return false;
        }
        accumulated = accumulated * 10ul + digit;
    }

    *value = accumulated;
    return true;
}

char *string_to_lower(char *text) {
    char *cursor;

    if (text == NULL) {
        return NULL;
    }

    for (cursor = text; *cursor != '\0'; cursor++) {
        *cursor = (char)tolower((unsigned char)*cursor);
    }

    return text;
}

char *string_to_upper(char *text) {
    char *cursor;

    if (text == NULL) {
        return NULL;
    }

    for (cursor = text; *cursor != '\0'; cursor++) {
        *cursor = (char)toupper((unsigned char)*cursor);
    }

    return text;
}

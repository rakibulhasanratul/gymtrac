#ifndef GYMTRAC_STRING_UTIL_H
#define GYMTRAC_STRING_UTIL_H

#include <stdbool.h>

#include "../settings.h"

/**
 * Copies text into destination with leading and trailing whitespace removed.
 *
 * The result is truncated when the trimmed string does not fit in
 * destination. Trimming never modifies text.
 *
 * @param text the string to trim
 * @param destination receives the trimmed string
 * @param destination_capacity the number of characters destination can hold
 */
void trim(const char text[], char *destination, int destination_capacity);

/**
 * Splits text on delimiter, copying each field into its own part buffer.
 *
 * Copies each field as a null-terminated string capped at
 * FIELD_BUFFER_SIZE - 1 characters; longer fields truncate. Consecutive and
 * trailing delimiters yield empty fields, so the part count is one more than
 * the delimiter count seen before capacity runs out.
 *
 * @param text the string to split; not modified
 * @param delimiter the character that separates fields
 * @param destination the 2D array that receives each captured field; every row
 *                    must hold exactly FIELD_BUFFER_SIZE characters, enforced
 *                    by the type
 * @param destination_capacity the number of rows available in destination
 * @return the number of parts captured; when this equals destination_capacity,
 *         text may still hold more unsplit fields
 */
int split(const char text[], char delimiter, char destination[][FIELD_BUFFER_SIZE], int destination_capacity);

/**
 * Converts text to a non-negative decimal number.
 *
 * @param text the string to convert
 * @return the parsed number, or 0 when text is NULL, empty, contains a
 *         non-digit, or overflows
 */
unsigned int string_to_unsigned_int(const char text[]);

/**
 * Converts text to a non-negative decimal number.
 *
 * @param text the string to convert
 * @return the parsed number, or 0 when text is NULL, empty, contains a
 *         non-digit, or overflows
 */
unsigned long int string_to_unsigned_long_int(const char text[]);

/**
 * Converts every letter in text to lowercase in place.
 *
 * @param text the string to convert
 * @return text, or NULL when text is NULL
 */
char *to_lowercase(char text[]);

/**
 * Converts every letter in text to uppercase in place.
 *
 * @param text the string to convert
 * @return text, or NULL when text is NULL
 */
char *to_uppercase(char text[]);

/**
 * Copies text into destination with control characters and the field delimiter
 * removed.
 *
 * The result is truncated when the sanitized string does not fit in
 * destination. Sanitizing never modifies text.
 *
 * @param text the field value to clean
 * @param destination receives the sanitized string
 * @param destination_capacity the number of characters destination can hold
 * @return true when the string was sanitized, false on invalid input
 */
bool sanitize_field(const char text[], char *destination, int destination_capacity);

/**
 * Checks whether text is NULL, empty, or contains only whitespace.
 *
 * Uses isspace() so a string of spaces, tabs, or newlines counts as blank.
 *
 * @param text the string to check
 * @return true when blank, false otherwise
 */
bool is_blank_string(const char text[]);

#endif

#ifndef GYMTRAC_STRING_UTIL_H
#define GYMTRAC_STRING_UTIL_H

#include <stdbool.h>

/**
 * Copies text into destination with leading and trailing whitespace removed.
 *
 * The result is truncated when the trimmed string does not fit in
 * destination. Trimming never modifies text.
 *
 * @param destination receives the trimmed string
 * @param destination_capacity the number of characters destination can hold
 * @param text the string to trim
 */
void trim(char destination[], int destination_capacity, const char text[]);

/**
 * Splits text on delimiter, copying each field into its own part buffer.
 *
 * Each field is copied as a null-terminated string of up to
 * field_capacity - 1 characters. Consecutive delimiters and a trailing
 * delimiter produce empty fields, so the part count is one more than the
 * number of delimiters seen before the capacity is reached.
 *
 * @param text the string to split; not modified
 * @param delimiter the character that separates fields
 * @param parts the array of buffers that receive each captured field; every
 *              buffer must hold at least field_capacity characters
 * @param part_capacity the number of buffers available in parts
 * @param field_capacity the number of characters each buffer can hold
 * @return the number of parts captured; when this equals part_capacity, text
 *         may still hold more unsplit fields
 */
int split(const char text[], char delimiter, char *parts[], int part_capacity, int field_capacity);

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
 * Removes control characters and the field delimiter from text in place.
 *
 * @param text the field value to clean
 * @return text, or NULL when text is NULL
 */
char *sanitize_field(char text[]);

#endif

#ifndef GYMTRAC_STRING_UTIL_H
#define GYMTRAC_STRING_UTIL_H

#include <stdbool.h>

/**
 * Trims leading and trailing whitespace from text in place.
 *
 * @param text the string to trim
 * @return the pointer to the first non-whitespace character in text, or NULL
 *         when text is NULL
 */
char *string_trim(char *text);

/**
 * Splits text in place on delimiter into a caller-provided array of parts.
 *
 * Each delimiter is replaced with a null terminator, so the parts point into
 * text and stay valid until text is modified.
 *
 * @param text the string to split; modified in place
 * @param delimiter the character that separates fields
 * @param parts the array that receives pointers to each captured part
 * @param part_capacity the number of slots available in parts
 * @return the number of parts captured; when this equals part_capacity, text
 *         may still hold more unsplit fields
 */
int string_split(char *text, char delimiter, char **parts, int part_capacity);

/**
 * Parses text as a whole non-negative decimal number into value.
 *
 * @param text the string to parse
 * @param value receives the parsed number on success
 * @return true when text is a valid number that fits unsigned int, false when
 *         text is NULL, empty, contains a non-digit, or overflows
 */
bool string_parse_unsigned(const char *text, unsigned int *value);

/**
 * Parses text as a whole non-negative decimal number into value.
 *
 * @param text the string to parse
 * @param value receives the parsed number on success
 * @return true when text is a valid number that fits unsigned long int, false
 *         when text is NULL, empty, contains a non-digit, or overflows
 */
bool string_parse_unsigned_long(const char *text, unsigned long int *value);

/**
 * Converts every letter in text to lowercase in place.
 *
 * @param text the string to convert
 * @return text, or NULL when text is NULL
 */
char *string_to_lower(char *text);

/**
 * Converts every letter in text to uppercase in place.
 *
 * @param text the string to convert
 * @return text, or NULL when text is NULL
 */
char *string_to_upper(char *text);

#endif

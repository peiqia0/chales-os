#include "string.h"
#include "stdint.h"
// i am way too tired to understand what i did last night so i will use ai to generate comments

/**
 * Finds the first occurrence of a character in a string.
 *
 * @param str The string to search for the character.
 * @param chr The character to search for.
 *
 * @return The pointer to the first occurrence of the character in the string, or NULL if the character was not found or if the string is NULL.
 */
const char* strchr(const char* str, char chr)
{
    if (str == NULL)
        return NULL;

    while (*str)
    {
        if (*str == chr)
            return str;

        ++str;
    }

    return NULL;
}

/**
 * Copies a string from source to destination.
 *
 * @param dst The destination string pointer.
 * @param src The source string pointer.
 *
 * @return The destination string pointer.
 *
 * @note If the destination pointer is NULL, the function returns NULL.
 * @note If the source pointer is NULL, the destination string is cleared.
 */
char* strcpy(char* dst, const char* src)
{
    char* origDst = dst;

    if (dst == NULL)
        return NULL;

    if (src == NULL)
    {
        *dst = '\0';
        return dst;
    }

    while (*src)
    {
        *dst = *src;
        ++src;
        ++dst;
    }
    
    *dst = '\0';
    return origDst;
}

/**
 * Calculates the length of a string.
 *
 * @param str The string to calculate the length of.
 *
 * @return The length of the string.
 *
 * @note If the string is NULL, the function returns 0.
 */
unsigned strlen(const char* str)
{
    unsigned len = 0;
    while (*str)
    {
        ++len;
        ++str;
    }

    return len;
}

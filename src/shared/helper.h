#pragma once

#include <stddef.h>
#include <string.h>

/**
 * stolen from an old project of mine
 */

static inline char *strccpy(char *dest, const char *src, size_t dest_size)
{
    size_t len = strlen(src);
    if (len >= dest_size)
    {
        len = dest_size - 1; // take 0 for the null terminator
    }

    memcpy(dest, src, len);
    dest[len] = '\0';

    return dest;
}

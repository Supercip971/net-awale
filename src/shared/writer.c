#include "writer.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void w_init(Writer *writer)
{
    *writer = (Writer){0};
    writer->data = malloc(64);
    writer->cap = 64;
    writer->len = 0;
    memset(writer->data, 0, 64);
}
void w_deinit(Writer *writer)
{
    free(writer->data);
    *writer = (Writer){0};
}

char const *w_view(Writer *writer)
{

    writer->data[writer->len + 1] = '\0';
    return writer->data;
}

size_t w_write(Writer *writer, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    char *temp_buffer = malloc(2048);

    size_t len = vsnprintf(temp_buffer, 2048, fmt, args);

    va_end(args);
    while (writer->cap < writer->len + 1 + len)
    {
        // mathematically known that cap is more performant when doing new_size*1.5+1
        writer->cap = writer->cap + (writer->cap / 2) + 1;
        writer->data = realloc(writer->data, writer->cap);
    }

    memcpy(writer->data + writer->len, temp_buffer, len);
    writer->len += len;
    free(temp_buffer);

    return len;
}

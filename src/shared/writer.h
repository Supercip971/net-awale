#pragma once

#include <stddef.h>

typedef struct Writer
{
    char *data;
    size_t cap;
    size_t len;
} Writer;

void w_init(Writer *writer);
void w_deinit(Writer *writer);
const char *w_view(Writer *writer);

size_t w_write(Writer *writer, const char *fmt, ...);

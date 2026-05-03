#pragma once

#include <stddef.h>

/// Réduit UTF-8 (latin / accents français) vers ASCII affichable sur police GFX 7 bits.
void text_utf8_fold_latin(const char* src, char* dst, size_t dst_size);

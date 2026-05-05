#pragma once

#include <stddef.h>

/// Fold UTF-8 Latin text into ASCII for the 7-bit GFX bitmap font.
void text_utf8_fold_latin(const char* src, char* dst, size_t dst_size);

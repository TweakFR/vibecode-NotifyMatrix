#include "text_utf8_fold.h"

#include <cstring>

void text_utf8_fold_latin(const char* src, char* dst, size_t dst_size)
{
  if (dst_size == 0) {
    return;
  }
  size_t wi = 0;
  for (size_t ri = 0; src[ri] != '\0' && wi + 1 < dst_size;) {
    const unsigned char c = static_cast<unsigned char>(src[ri]);
    if (c < 0x80u) {
      dst[wi++] = static_cast<char>(c);
      ri++;
      continue;
    }
    if (c == 0xC2u && src[ri + 1] != '\0') {
      const unsigned char c2 = static_cast<unsigned char>(src[ri + 1]);
      dst[wi++] = (c2 == 0xA0u) ? ' ' : '?';
      ri += 2;
      continue;
    }
    if (c == 0xC3u && src[ri + 1] != '\0') {
      const unsigned char b = static_cast<unsigned char>(src[ri + 1]);
      char out = '?';
      if (b >= 0x80u && b <= 0x85u) {
        out = 'A';
      } else if (b == 0x86u) {
        out = 'A';
      } else if (b == 0x87u) {
        out = 'C';
      } else if (b >= 0x88u && b <= 0x8Bu) {
        out = 'E';
      } else if (b >= 0x8Cu && b <= 0x8Fu) {
        out = 'I';
      } else if (b == 0x90u) {
        out = 'D';
      } else if (b == 0x91u) {
        out = 'N';
      } else if (b >= 0x92u && b <= 0x96u) {
        out = 'O';
      } else if (b == 0x98u) {
        out = 'O';
      } else if (b >= 0x99u && b <= 0x9Cu) {
        out = 'U';
      } else if (b == 0x9Du) {
        out = 'Y';
      } else if (b == 0x9Fu) {
        out = 's';
      } else if (b >= 0xA0u && b <= 0xA5u) {
        out = 'a';
      } else if (b == 0xA6u) {
        out = 'a';
      } else if (b == 0xA7u) {
        out = 'c';
      } else if (b >= 0xA8u && b <= 0xABu) {
        out = 'e';
      } else if (b >= 0xACu && b <= 0xAFu) {
        out = 'i';
      } else if (b == 0xB1u) {
        out = 'n';
      } else if (b >= 0xB2u && b <= 0xB6u) {
        out = 'o';
      } else if (b == 0xB8u) {
        out = 'o';
      } else if (b >= 0xB9u && b <= 0xBCu) {
        out = 'u';
      } else if (b == 0xBFu) {
        out = 'y';
      }
      dst[wi++] = out;
      ri += 2;
      continue;
    }
    if ((c & 0xE0u) == 0xC0u && src[ri + 1] != '\0') {
      ri += 2;
      dst[wi++] = '?';
      continue;
    }
    if ((c & 0xF0u) == 0xE0u && src[ri + 1] != '\0' && src[ri + 2] != '\0') {
      ri += 3;
      dst[wi++] = '?';
      continue;
    }
    if ((c & 0xF8u) == 0xF0u && src[ri + 1] != '\0' && src[ri + 2] != '\0' &&
        src[ri + 3] != '\0') {
      ri += 4;
      dst[wi++] = '?';
      continue;
    }
    ri++;
    dst[wi++] = '?';
  }
  dst[wi] = '\0';
}

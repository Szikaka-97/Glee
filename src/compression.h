#pragma once

// https://www.experts-exchange.com/articles/3189/In-Memory-Compression-and-Decompression-Using-ZLIB.html

#define ZLIB_WINAPI

#include "binary.h"
#include "zlib.h"

int GetMaxCompressedLen(int nLenSrc);

int CompressData(const byte* abSrc, int nLenSrc, byte* abDst, int nLenDst);

int UncompressData(const byte* abSrc, int nLenSrc, byte* abDst, int nLenDst);
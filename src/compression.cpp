#include "compression.h"


int GetMaxCompressedLen(int nLenSrc) {
	int n16kBlocks = (nLenSrc + 16383) / 16384; // round up any fraction of a block
	return nLenSrc + 6 + (n16kBlocks*5);
}

int CompressData(const byte* abSrc, int nLenSrc, byte* abDst, int nLenDst) {
	z_stream zInfo = {0};
	zInfo.total_in =  zInfo.avail_in =  nLenSrc;
	zInfo.total_out = zInfo.avail_out = nLenDst;
	zInfo.next_in = (byte*) abSrc;
	zInfo.next_out = abDst;

	int nErr, nRet = -1;
	nErr = deflateInit(&zInfo, 5);
	if (nErr == Z_OK) {
		nErr= deflate(&zInfo, Z_FINISH);

		if (nErr == Z_STREAM_END) {
			nRet = zInfo.total_out;
		}
	}
	deflateEnd(&zInfo);

	return nRet;
}

int UncompressData(const byte* abSrc, int nLenSrc, byte* abDst, int nLenDst) {
	z_stream zInfo = {0};
	zInfo.total_in = zInfo.avail_in = nLenSrc;
	zInfo.total_out = zInfo.avail_out = nLenDst;
	zInfo.next_in = (byte*) abSrc;
	zInfo.next_out = abDst;

	int nErr, nRet = -1;
	nErr = inflateInit( &zInfo );
	if (nErr == Z_OK) {
		nErr = inflate(&zInfo, Z_FINISH);

		if (nErr == Z_STREAM_END) {
			nRet = zInfo.total_out;
		}
	}
	inflateEnd(&zInfo);

	return nRet; // -1 or len of output
}
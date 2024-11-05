#pragma once

#include <vector>
#include <filesystem>

#include "binary.h"

class DCXFile {
private:
	size_t compressedSize;
	size_t uncompressedSize;
	size_t compressedHeaderLength;

	byte* compressedFileData;

public:
	DCXFile(size_t compressedSize, size_t uncompressedSize, size_t compressedHeaderLength, byte* fileData);
	~DCXFile();
	
	static DCXFile* ReadFile(const std::filesystem::path& filePath);
	static DCXFile* Pack(byte* fileData, size_t dataLength, size_t compressedHeaderLength = 8);

	byte* Decompress(size_t& decompressedSize);

	void WriteFile(const std::filesystem::path& filePath);

	size_t GetCompressedSize() { return this->compressedSize; }
	size_t GetUncompressedSize() { return this->uncompressedSize; }
	size_t GetCompressedHeaderLength() { return this->compressedHeaderLength; }
	const byte* GetCompressedFileData() { return this->compressedFileData; }
};
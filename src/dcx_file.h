#pragma once

#include <vector>
#include <filesystem>

#include "binary.h"

class BNDFile;

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

	byte* Decompress(size_t& decompressedSize) const;

	void WriteFile(const std::filesystem::path& filePath);

	size_t GetCompressedSize() { return this->compressedSize; }
	size_t GetUncompressedSize() { return this->uncompressedSize; }
	size_t GetCompressedHeaderLength() { return this->compressedHeaderLength; }
	const byte* GetCompressedFileData() { return this->compressedFileData; }
};
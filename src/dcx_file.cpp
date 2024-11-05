#include "dcx_file.h"

#include <array>
#include <cstring>
#include <fstream>

#include "spdlog/spdlog.h"

#include "compression.h"

#define RETURN_ERR file.close(); return nullptr;

namespace fs = std::filesystem;

const size_t dcx_header_size = 0x4C;

DCXFile::DCXFile(size_t compressedSize, size_t uncompressedSize, size_t compressedHeaderLength, byte* fileData):
	compressedSize(compressedSize),
	uncompressedSize(uncompressedSize),
	compressedHeaderLength(compressedHeaderLength),
	compressedFileData(fileData) { }

DCXFile::~DCXFile() {
	delete[] this->compressedFileData;
}

DCXFile* DCXFile::ReadFile(const fs::path& filePath) {
	std::basic_ifstream<char> file(filePath, std::ios::binary);

	if (!file.is_open()) {
		spdlog::error("Cannot open file: " + filePath.string());

		RETURN_ERR;
	}

	byte header[dcx_header_size] = {0};

	file.read((char *) header, dcx_header_size);

	if (!file.good()) {
		spdlog::error("Unable to read header : " + filePath.string());

		RETURN_ERR;
	}

	int counter = 0;

	if (!AssertASCII(ExtractASCII<4>(header, counter), "DCX", "Magic Value")) {
		RETURN_ERR;
	}

	if (!AssertInt(ExtractInt32(header, counter), 0x11000, "Type specific unknown at 0x04")) {
		RETURN_ERR;
	}

	if (!AssertInt(ExtractInt32(header, counter), 0x18, "Unknown at 0x08")) {
		RETURN_ERR;
	}

	if (!AssertInt(ExtractInt32(header, counter), 0x24, "Unknown at 0x0C")) {
		RETURN_ERR;
	}

	if (!AssertInt(ExtractInt32(header, counter), 0x44, "Type specific unknown at 0x10")) {
		RETURN_ERR;
	}

	if (!AssertInt(ExtractInt32(header, counter), 0x4C, "Type specific unknown at 0x14")) {
		RETURN_ERR;
	}

	if (!AssertASCII(ExtractASCII<4>(header, counter), "DCS", "DCS magic")) {
		RETURN_ERR;
	}

	int uncompressedSize = ExtractInt32(header, counter);
	int compressedSize = ExtractInt32(header, counter);

	if (!AssertASCII(ExtractASCII<4>(header, counter), "DCP", "DCP magic")) {
		RETURN_ERR;
	}

	if (!AssertASCII(ExtractASCII<4>(header, counter), "DFLT", "DFLT magic")) {
		RETURN_ERR;
	}

	if (!AssertInt(ExtractInt32(header, counter), 0x20, "Unknown at 0x2C")) {
		RETURN_ERR;
	}

	if (!AssertInt(ExtractInt32(header, counter), 0x9000000, "Type specific unknown at 0x30")) {
		RETURN_ERR;
	}

	if (!AssertInt(ExtractInt32(header, counter), 0, "Type specific unknown at 0x34")) {
		RETURN_ERR;
	}
	if (!AssertInt(ExtractInt32(header, counter), 0, "Type specific unknown at 0x38")) {
		RETURN_ERR;
	}
	if (!AssertInt(ExtractInt32(header, counter), 0, "Type specific unknown at 0x3C")) {
		RETURN_ERR;
	}

	if (!AssertInt(ExtractInt32(header, counter), 0x00010100, "Unknown at 0x40")) {
		RETURN_ERR;
	}

	if (!AssertASCII(ExtractASCII<4>(header, counter), "DCA", "DCA magic")) {
		RETURN_ERR;
	}

	int compressedHeaderLength = ExtractInt32(header, counter);

	byte* fileData = new byte[compressedSize];

	file.read((char *) fileData, compressedSize);

	file.close();

	return new DCXFile(compressedSize, uncompressedSize, compressedHeaderLength, fileData);
}

DCXFile* DCXFile::Pack(byte* fileData, size_t dataLength, size_t compressedHeaderLength) {
	// It's usually enough for the file with ~500K to spare
	const size_t expectedCompressedSize = 5e6;

	byte* compressedData = new byte[expectedCompressedSize];

	int actualCompressedSize = CompressData(fileData, dataLength, compressedData, expectedCompressedSize);

	return new DCXFile(actualCompressedSize, dataLength, compressedHeaderLength, compressedData);
}

byte* DCXFile::Decompress(size_t& decompressedSize) {
	byte* buffer = new byte[this->uncompressedSize];

	int result = UncompressData(this->compressedFileData, this->compressedSize, buffer, this->uncompressedSize);

	if (result < 0) {
		delete[] buffer;

		return nullptr;
	}
	else {
		decompressedSize = result;

		return buffer;
	}
}

void DCXFile::WriteFile(const fs::path& filePath) {
	std::ofstream file(filePath, std::ios::binary);

	file << ToBytes<4>("DCX");
	file << ToBytes(0x11000);
	file << ToBytes(0x18);
	file << ToBytes(0x24);
	file << ToBytes(0x44);
	file << ToBytes(0x4C);
	file << ToBytes<4>("DCS");
	file << ToBytes(this->uncompressedSize);
	file << ToBytes(this->compressedSize);
	file << ToBytes<4>("DCP");
	file << ToBytes<4>("DFLT");
	file << ToBytes(0x20);
	file << ToBytes(0x9000000);
	file << ToBytes(0);
	file << ToBytes(0);
	file << ToBytes(0);
	file << ToBytes(0x00010100);
	file << ToBytes<4>("DCA");
	file << ToBytes(this->compressedHeaderLength);

	file.write((char *) this->compressedFileData, this->compressedSize);

	file.close();
}
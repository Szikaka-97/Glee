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

	BufferView headerView(header, dcx_header_size);

	try {
		headerView.AssertASCII("DCX", 4, "Magic Value");
		headerView.AssertInt32(0x11000, "");
		headerView.AssertInt32(0x18, "");
		headerView.AssertInt32(0x24, "");
		headerView.AssertInt32(0x44, "");
		headerView.AssertInt32(0x4C, "");
		headerView.AssertASCII("DCS", 4, "DCS magic");

		int uncompressedSize = headerView.ReadInt32();
		int compressedSize = headerView.ReadInt32();

		headerView.AssertASCII("DCP", 4, "DCP magic");
		headerView.AssertASCII("DFLT", "DFLT magic");
		headerView.AssertInt32(0x20, "");
		headerView.AssertInt32(0x9000000, "");
		headerView.AssertInt32(0, "");
		headerView.AssertInt32(0, "");
		headerView.AssertInt32(0, "");
		headerView.AssertInt32(0x00010100, "");
		headerView.AssertASCII("DCA", "DCA magic");

		int compressedHeaderLength = headerView.ReadInt32();

		byte* fileData = new byte[compressedSize];

		file.read((char *) fileData, compressedSize);

		file.close();

		return new DCXFile(compressedSize, uncompressedSize, compressedHeaderLength, fileData);
	} catch (std::runtime_error e) {
		spdlog::error(e.what());

		file.close();

		return nullptr;
	}
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
#include "bnd_file.h"

#include <vector>
#include <fstream>
#include <chrono>

namespace stdtime = std::chrono;

#include "binary.h"
#include "dcx_file.h"
#include "compression.h"

struct BNDFileHeaderInfo {
	int dataOffset;
	int size;
	std::string name;
};

BNDFile::BNDFile(byte* backingData, size_t size):
	backingData(backingData),
	fileSize(size) {}

BNDFile::~BNDFile() {
	for (auto& segment : this->matbinFileOffsets) {
		if (segment.second.loaded) {
			delete segment.second.matbin;
		}
	}

	delete[] backingData;
}

// Always does
bool HasCompression(byte format) {
	return format & 0b00100000;
}

// Always does
bool HasLongOffsets(byte format) {
	return format & 0b00010000;
}

// Never does
bool HasIDs(byte format) {
	return format & 0b00000010;
}

// Always does
bool HasNames(byte format) {
	return format & 0b00001100;
}

byte DecodeFlags(byte readFormat, bool reverseBits) {
	if (reverseBits || readFormat & 1 && !(readFormat & 0b10000000)) {
		byte result = 0;

		for (int i = 0; i < 8; i++) {
			result |= 128 * (readFormat & 1) >> i;

			readFormat >>= 1;
		}

		return result;
	}
	else {
		return readFormat;
	}
}

BNDFileHeaderInfo ReadBNDFileHeader(BufferView& dataView, byte format, bool reverseBits) {
	byte flags = DecodeFlags(dataView.ReadBoolean(), reverseBits);
	dataView.AssertByte(0, "Padding");
	dataView.AssertByte(0, "Padding");
	dataView.AssertByte(0, "Padding");
	dataView.AssertInt32(-1, "Padding");

	int fileSize = dataView.ReadInt64();

	int uncompressedSize = fileSize;
	if (HasCompression(format)) {
		uncompressedSize = dataView.ReadInt64();
	}

	int dataOffset = 0;
	if (HasLongOffsets(format)) {
		dataOffset = dataView.ReadInt64();
	}
	else {
		dataOffset = dataView.ReadInt32();
	}

	int id = -1;
	if (HasIDs(format)) {
		id = dataView.ReadInt32();
	}

	int nameOffset = 0;

	std::string name;
	if (HasNames(format)) {
		nameOffset = dataView.ReadInt32();

		// Technically, the names can be either UTF16 or Shift JIS, but in the material file they are always UTF16

		name = dataView.ReadOffsetUTF16(nameOffset);

		if (name.ends_with(".matbin")) {
			int nameStart = name.find_last_of("\\") + 1;

			// 7 for the length of ".matbin"
			name = name.substr(nameStart, name.length() - nameStart - 7);
		}
	}

	return {dataOffset, fileSize, name};
}

BNDFile* BNDFile::Parse(const byte* data, size_t dataLength) {
	try {
		BufferView dataView(data, dataLength, false);

		dataView.AssertASCII("BND4", "Magic Value");

		bool unk04 = dataView.ReadBoolean();
		bool unk05 = dataView.ReadBoolean();
		dataView.AssertByte(0, "Padding 1");
		dataView.AssertByte(0, "Padding 2");
		dataView.AssertByte(0, "Padding 3");
		bool bigEndian = dataView.ReadBoolean();
		bool reverseBits = !dataView.ReadBoolean();
		dataView.AssertByte(0, "Padding 4");

		dataView.SetBigEndian(bigEndian);

		int fileCount = dataView.ReadInt32();
		dataView.AssertInt64(0x40, "Header size");
		std::array<byte, 8> version = dataView.Read<8>();

		int fileHeaderSize = dataView.ReadInt64();

		dataView.Skip<uint64_t>();

		bool unicode = dataView.ReadBoolean();
		byte format = DecodeFlags(dataView.ReadByte(), reverseBits);

		byte extended = dataView.ReadByte();

		dataView.AssertByte(0, "Padding 5");
		dataView.AssertInt32(0, "Padding 6");

		if (extended == 4) { // We actually always take this branch
			dataView.Advance(8);

			// SoulsFormats does some shit here, we don't actually need it
		}
		else {
			dataView.AssertInt64(0, "Hash table size");
		}

		BNDFile* result = new BNDFile(const_cast<byte *>(data), dataLength);
		result->unk04 = unk04;
		result->unk05 = unk05;
		result->bigEndian = bigEndian;
		result->bitBigEndian = reverseBits;
		result->version = version;
		result->unicode = unicode;
		result->format = format;
		result->extended = extended;

		for (int i = 0; i < fileCount; i++) {
			auto fileHeader = ReadBNDFileHeader(dataView, format, reverseBits);

			result->matbinFileOffsets[fileHeader.name] = MatbinSegment(const_cast<byte *>(data + fileHeader.dataOffset), fileHeader.size);
		}

		return result;
	} catch (const std::runtime_error& e) {
		spdlog::error(std::format("Runtime: {}", e.what()));

		return nullptr;
	}
}

BNDFile* BNDFile::Unpack(const DCXFile* file) {
	size_t actualDecompressedSize = 0;
	byte* outFileBuffer = file->Decompress(actualDecompressedSize);

	auto result = BNDFile::Parse(outFileBuffer, actualDecompressedSize);

	if (!result) {
		delete outFileBuffer;

		return nullptr;
	}
	else {
		return result;
	}
}

DCXFile* BNDFile::Pack(size_t compressedHeaderLength) {
	// It's usually enough for the file with ~500K to spare
	const size_t expectedCompressedSize = 5e6;

	byte* compressedData = new byte[expectedCompressedSize];

	int actualCompressedSize = CompressData(this->backingData, this->fileSize, compressedData, expectedCompressedSize);

	return new DCXFile(actualCompressedSize, this->fileSize, compressedHeaderLength, compressedData);
}

MatbinFile* BNDFile::GetMatbin(std::string name) {
	if (this->matbinFileOffsets.contains(name)) {
		auto& segmentInfo = this->matbinFileOffsets[name];

		if (!segmentInfo.loaded) {
			auto offsetInfo = segmentInfo.dataLocation;

			segmentInfo.matbin = new MatbinFile(offsetInfo.start, offsetInfo.length);
			segmentInfo.loaded = true;
		}

		return segmentInfo.matbin;
	}

	return nullptr;
}

void BNDFile::ApplyMod(const MaterialMod& mod) {
	const auto changes = mod.GetChanges();

	for (const auto& change : changes) {
		if (this->matbinFileOffsets.contains(change.first)) {
			spdlog::info(std::format("Modding material {}", change.first));

			this->GetMatbin(change.first)->ApplyMod(*change.second);
		}
	}
}
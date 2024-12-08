#include "bnd_file.h"

#include <vector>
#include <fstream>
#include <chrono>

namespace stdtime = std::chrono;

#include "binary.h"
#include "dcx_file.h"
#include "compression.h"

BNDFile::BNDFile(byte* backingData, size_t size):
	backingData(backingData),
	fileSize(size) {}

BNDFile::~BNDFile() {
	for (auto& segment : this->bindedFileInfos) {
		if (segment.loaded) {
			delete segment.matbin;
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

// Always does
bool HasNames(byte format) {
	return format & 0b00001100;
}

byte DecodeFlags(byte readFormat, bool reverseBits) {
	if (reverseBits || readFormat & 1 && !(readFormat & 0b10000000)) {
		byte result = 0;

		for (int i = 0; i < 8; i++) {
			result |= (128 * (readFormat & 1)) >> i;

			readFormat >>= 1;
		}

		return result;
	}
	else {
		return readFormat;
	}
}

constexpr int BindedFileInfo::GetSize() {
	return (
		1 // flags
		+ 3 // padding
		+ 4 // dataView.AssertInt32(-1);
		+ 8 // int fileSize = dataView.ReadInt64();
		+ 8	// int uncompressedSize = dataView.ReadInt64();
		+ 8 // int dataOffset = dataView.ReadInt64();
		+ 4// int pathOffset = dataView.ReadInt32();
	);
}

std::string BindedFileInfo::GetName() const {
	std::filesystem::path networkPath = this->path;

	return PathUtils::StemWindows(networkPath).string();
}

std::string BindedFileInfo::GetNameWithParentFolder() const {
	std::filesystem::path networkPath = this->path;
	networkPath = PathUtils::ChangeSlashes(networkPath);
	
	return (networkPath.parent_path().stem() / networkPath.stem()).string();
}


BindedFileInfo BNDFile::ReadBNDFileHeader(BufferView& dataView, byte format, bool reverseBits) {
	// I am assuming a format of 01110100, since that's what all these have
	byte flags = DecodeFlags(dataView.ReadByte(), reverseBits);
	dataView.AssertByte(0);
	dataView.AssertByte(0);
	dataView.AssertByte(0);
	dataView.AssertInt32(-1);

	int fileSize = dataView.ReadInt64();

	int uncompressedSize = dataView.ReadInt64();
	int dataOffset = dataView.ReadInt64();

	int pathOffset = dataView.ReadInt32();
	std::string filePath = dataView.ReadOffsetUTF16(pathOffset);
	byte* fileContent = new byte[uncompressedSize];

	memcpy(fileContent, this->backingData + dataOffset, uncompressedSize);

	return BindedFileInfo(filePath, flags, fileContent, uncompressedSize);
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
		uint64_t version = dataView.ReadInt64();

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

		BNDFileHeader header {
			unk04,
			unk05,
			bigEndian,
			reverseBits,
			fileCount,
			version,
			unicode,
			format
		};

		BNDFile* result = new BNDFile(const_cast<byte *>(data), dataLength);

		result->header = header;
		result->bindedFileInfos.reserve(fileCount);
		for (int i = 0; i < fileCount; i++) {
			auto fileHeader = result->ReadBNDFileHeader(dataView, format, reverseBits);

			result->bindedFileInfos.push_back(fileHeader);

			if (result->matbinFileMap.contains(fileHeader.GetName())) {
				auto newName = fileHeader.GetNameWithParentFolder();

				spdlog::warn("Path conflict: {} and {}, the second one will be saved as {}", result->matbinFileMap[fileHeader.GetName()]->path, fileHeader.path, newName);

				result->matbinFileMap[newName] = &result->bindedFileInfos[i];
			}
			else {
				result->matbinFileMap[fileHeader.GetName()] = &result->bindedFileInfos[i];
			}
		}

		return result;
	} catch (const std::runtime_error& e) {
		spdlog::error("Caught exception when parsing the BND file: {}", e.what());

		return nullptr;
	}
}

void BNDFile::Write(const std::filesystem::path& dest) {
	std::ofstream file(dest, std::ios::binary);

	file << ToBytes<4>("BND4");

	file << ToBytes(this->header.unk04);
	file << ToBytes(this->header.unk05);
	file << ToBytes((bool) 0);
	file << ToBytes((bool) 0);
	file << ToBytes((bool) 0);
	file << ToBytes(this->header.bigEndian);
	file << ToBytes(!this->header.reverseFlagBits);
	file << ToBytes((bool) 0);
	file << ToBytes((int) this->bindedFileInfos.size(), this->header.bigEndian);
	file << ToBytes((uint64_t) 0x40, this->header.bigEndian);
	file << ToBytes(this->header.version, this->header.bigEndian);
	file << ToBytes((uint64_t) BindedFileInfo::GetSize(), this->header.bigEndian);
	file << ToBytes((uint64_t) 0);
	file << ToBytes(this->header.unicode);
	file << ToBytes(DecodeFlags(this->header.format, this->header.reverseFlagBits));
	file << ToBytes((byte) 4);
	file << ToBytes((byte) 0);
	file << ToBytes((int) 0);
	file << ToBytes((uint64_t) 0);

	size_t headersStartOffset = file.tellp();
	size_t baseOffset = headersStartOffset + this->bindedFileInfos.size() * BindedFileInfo::GetSize();
	size_t nameOffset = 0;

	for (const auto& bindedFile : this->bindedFileInfos) {
		file << ToBytes(DecodeFlags(0b01000000, this->header.reverseFlagBits));
		file << ToBytes((byte) 0);
		file << ToBytes((byte) 0);
		file << ToBytes((byte) 0);
		file << ToBytes(-1, this->header.bigEndian);
		file << ToBytes(bindedFile.GetFileSize(), this->header.bigEndian);
		file << ToBytes(bindedFile.GetFileSize(), this->header.bigEndian);
		file << ToBytes((int64_t) -1); // File offset
		file << ToBytes((int) (baseOffset + nameOffset), this->header.bigEndian); // Name offset

		nameOffset += (bindedFile.path.size() + 1) * 2;
	}

	for (const auto& bindedFile : this->bindedFileInfos) {
		WriteUTF16ToStream(file, bindedFile.path);
	}

	int i = 0;
	for (const auto& bindedFile : this->bindedFileInfos) {
		auto currentPos = file.tellp();

		file.seekp(headersStartOffset + (i * BindedFileInfo::GetSize()) + 24);

		file << ToBytes((int64_t) currentPos, this->header.bigEndian);

		file.seekp(currentPos);

		if (bindedFile.loaded) {
			file.write((char *) bindedFile.matbin->GetStart(), bindedFile.matbin->GetLength());
		}
		else {
			file.write((char *) bindedFile.dataLocation.start, bindedFile.dataLocation.length);
		}

		i++;
	}

	file.close();
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

void BNDFile::Relocate() {
	byte* newLocation = new byte[this->GetSize()];
	BufferView data(newLocation, this->GetSize(), true);

	data.WriteASCII("BND4", false);
	data.WriteBool(this->header.unk04);
	data.WriteBool(this->header.unk05);
	data.Write<3>({0});
	data.WriteBool(this->header.bigEndian);
	data.WriteBool(!this->header.reverseFlagBits);
	data.WriteBool(0);
	data.SetBigEndian(this->header.bigEndian);
	data.WriteInt32(this->bindedFileInfos.size());
	data.WriteInt64(0x40);
	data.WriteInt64(this->header.version);
	data.WriteInt64(BindedFileInfo::GetSize());
	data.WriteInt64(0);
	data.WriteBool(this->header.unicode);
	data.WriteByte(DecodeFlags(this->header.format, this->header.reverseFlagBits));
	data.WriteByte(4);
	data.WriteByte(0);
	data.WriteInt32(0);
	data.WriteInt64(0);

	size_t headersStartOffset = data.GetOffset();
	size_t baseOffset = headersStartOffset + this->bindedFileInfos.size() * BindedFileInfo::GetSize();
	size_t pathOffset = 0;

	for (const auto& bindedFile : this->bindedFileInfos) {
		data.WriteByte(DecodeFlags(0b01000000, this->header.reverseFlagBits));
		data.Write<3>({0});
		data.WriteInt32(-1);
		data.WriteInt64(bindedFile.GetFileSize());
		data.WriteInt64(bindedFile.GetFileSize());
		data.WriteInt64(-1); // File offset, we'll get to it later
		data.WriteInt32(baseOffset + pathOffset); // Path offset

		pathOffset += (bindedFile.path.size() + 1) * 2;
	}

	for (const auto& bindedFile : this->bindedFileInfos) {
		data.WriteUTF16(bindedFile.path);
	}

	int i = 0;
	for (const auto& bindedFile : this->bindedFileInfos) {
		auto currentPos = data.GetOffset();

		data.SetOffset(headersStartOffset + (i * BindedFileInfo::GetSize()) + 24);

		data.WriteInt64(currentPos);

		data.SetOffset(currentPos);

		if (bindedFile.loaded) {
			data.WriteData(bindedFile.matbin->GetStart(), bindedFile.matbin->GetLength());
		}
		else {
			data.WriteData(bindedFile.dataLocation.start, bindedFile.dataLocation.length);
		}

		i++;
	}

	delete[] this->backingData;

	this->backingData = newLocation;
}

DCXFile* BNDFile::Pack(size_t compressedHeaderLength) {
	if (this->sizeDelta != 0) {
		Relocate();
	}

	// It's usually enough for the file with ~500K to spare
	const size_t expectedCompressedSize = 5e6;

	byte* compressedData = new byte[expectedCompressedSize];

	int actualCompressedSize = CompressData(this->backingData, this->fileSize + this->sizeDelta, compressedData, expectedCompressedSize);

	return new DCXFile(actualCompressedSize, this->fileSize + this->sizeDelta, compressedHeaderLength, compressedData);
}

const std::vector<const std::string*> BNDFile::GetAllMatbinPaths(bool fullPaths) {
	std::vector<const std::string*> pathList;
	pathList.reserve(this->matbinFileMap.size());

	for (const auto& pair : this->matbinFileMap) {
		if (fullPaths) {
			pathList.push_back(&pair.second->path);
		}
		else {
			pathList.push_back(&pair.first);
		}
	}

	return pathList;
}

MatbinFile* BNDFile::GetMatbin(std::string name) {
	if (this->matbinFileMap.contains(name)) {
		auto segmentInfo = this->matbinFileMap[name];

		if (!segmentInfo->loaded) {
			auto offsetInfo = segmentInfo->dataLocation;

			segmentInfo->matbin = new MatbinFile(offsetInfo.start, offsetInfo.length);
			segmentInfo->loaded = true;
		}

		return segmentInfo->matbin;
	}

	return nullptr;
}

void BNDFile::ApplyMod(const MaterialMod& mod) {
	const auto changes = mod.GetChanges();

	for (const auto& change : changes) {
		spdlog::info("Changes: {}", change.first);

		if (this->matbinFileMap.contains(change.first)) {
			spdlog::info("Modding material {}", change.first);

			auto matbin = this->GetMatbin(change.first);

			size_t oldLength = matbin->GetLength();

			matbin->ApplyMod(*change.second);

			if (matbin->WasRelocated()) {
				spdlog::info("Material {} was relocated", change.first);

				this->sizeDelta += matbin->GetLength() - oldLength;
			}
		}
	}

	spdlog::info("Overall size Change: {}", this->sizeDelta);
}

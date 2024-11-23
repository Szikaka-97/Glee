#pragma once

#include <map>
#include <string>

#include "binary.h"
#include "matbin_file.h"

#include "material_mod.h"

class DCXFile;

class BNDFile {
private:
	struct OldMatbinSegment {
		bool loaded;

		union {
			struct {
				byte* start;
				int length;
			} dataLocation;
			MatbinFile* matbin;
		};

		OldMatbinSegment():
		loaded(false),
		dataLocation{nullptr, 0} {}

		OldMatbinSegment(byte* start, int length):
		loaded(false),
		dataLocation{start, length} {}
	};

	struct BNDFileInfo {
		std::string path;
		byte format;
		bool loaded;

		union {
			struct {
				byte* start;
				int length;
			} dataLocation;
			MatbinFile* matbin;
		};

		BNDFileInfo():
			loaded(false),
			path(""),
			format(0),
			dataLocation{0, 0} {}

		BNDFileInfo(const std::string& path, byte format, byte* position, int length):
			loaded(false),
			path(path),
			format(format),
			dataLocation{position, length} {}

		std::string GetName() {
			std::filesystem::path networkPath = this->path;

			return PathUtils::StemWindows(networkPath).string();
		}
	};

	byte* backingData;
	size_t headerSize;
	size_t fileSize;
	std::map<std::string, BNDFileInfo> matbinFiles;

	bool unk04;
	bool unk05;
	bool bigEndian;
	bool bitBigEndian;
	std::array<byte, 8> version;
	bool unicode;
	byte format;
	byte extended;

	int sizeDelta = 0;

	BNDFile(byte* backingData, size_t size);

	BNDFileInfo ReadBNDFileHeader(BufferView& dataView, byte format, bool reverseBits);
	size_t GetBNDFileHeaderSize(const BNDFileInfo& fileInfo);
public:
	~BNDFile();

	static BNDFile* Parse(const byte* data, size_t dataLength);
	void Write(const std::filesystem::path& dest);

	void Relocate();

	DCXFile* Pack(size_t compressedHeaderLength = 8);
	static BNDFile* Unpack(const DCXFile* file);

	MatbinFile* GetMatbin(std::string name);

	void ApplyMod(const MaterialMod& mod);

	const byte* GetData() { return this->backingData; }
	size_t GetSize() { return this->fileSize; }
};
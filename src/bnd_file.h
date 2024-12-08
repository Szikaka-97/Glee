#pragma once

#include <map>
#include <string>

#include "binary.h"
#include "matbin_file.h"

#include "material_mod.h"

class DCXFile;

struct BNDFileHeader {
	bool unk04;
	bool unk05;
	bool bigEndian;
	bool reverseFlagBits;
	int fileCount;
	uint64_t version;
	bool unicode;
	byte format;
};

struct BindedFileInfo {
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

	BindedFileInfo():
		loaded(false),
		path(""),
		format(0),
		dataLocation{0, 0} {}

	BindedFileInfo(const std::string& path, byte format, byte* position, int length):
		loaded(false),
		path(path),
		format(format),
		dataLocation{position, length} {}

	std::string GetName() const;

	std::string GetNameWithParentFolder() const;

	uint64_t GetFileSize() const {
		if (this->loaded) {
			return this->matbin->GetLength();
		}
		else {
			return this->dataLocation.length;
		}
	}

	static constexpr int GetSize();
};

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
	size_t fileSize;
	std::vector<BindedFileInfo> bindedFileInfos;
	std::map<std::string, BindedFileInfo*> matbinFileMap;

	BNDFileHeader header;

	int sizeDelta = 0;

	BNDFile(byte* backingData, size_t size);

	BindedFileInfo ReadBNDFileHeader(BufferView& dataView, byte format, bool reverseBits);
public:
	~BNDFile();

	static BNDFile* Parse(const byte* data, size_t dataLength);
	void Write(const std::filesystem::path& dest);

	void Relocate();

	DCXFile* Pack(size_t compressedHeaderLength = 8);
	static BNDFile* Unpack(const DCXFile* file);

	const std::vector<const std::string*> GetAllMatbinPaths(bool fullPaths = false);

	MatbinFile* GetMatbin(std::string name);

	void ApplyMod(const MaterialMod& mod);

	const byte* GetData() { return this->backingData; }
	size_t GetSize() { return this->fileSize + this->sizeDelta; }
};
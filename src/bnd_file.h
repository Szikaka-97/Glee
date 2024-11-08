#pragma once

#include <map>
#include <string>

#include "binary.h"
#include "matbin_file.h"

#include "material_mod.h"

class BNDFile {
private:
	struct MatbinSegment {
		bool loaded;

		union {
			struct {
				byte* start;
				int length;
			} dataLocation;
			MatbinFile* matbin;
		};

		MatbinSegment():
		loaded(false),
		dataLocation{nullptr, 0} {}

		MatbinSegment(byte* start, int length):
		loaded(false),
		dataLocation{start, length} {}
	};

	const byte* backingData;
	std::map<std::string, MatbinSegment> matbinFileOffsets;

	bool unk04;
	bool unk05;
	bool bigEndian;
	bool bitBigEndian;
	std::array<byte, 8> version;
	bool unicode;
	byte format;
	byte extended;

	BNDFile(const byte* backingData);
public:
	~BNDFile();

	static BNDFile* Parse(const byte* data, size_t dataLength);

	MatbinFile* GetMatbin(std::string name);

	void ApplyMod(const MaterialMod& mod);
};
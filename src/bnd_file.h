#pragma once

#include <map>
#include <string>

#include "binary.h"
#include "matbin_file.h"

class BNDFile {
private:
	const byte* backingData;
	std::map<std::string, MatbinFile> matbinFileOffsets;

	BNDFile(const byte* backingData);
public:
	static BNDFile* Parse(const byte* data, size_t dataLength);

	MatbinFile GetMatbin(std::string name);
};
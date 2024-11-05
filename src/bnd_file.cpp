#include "bnd_file.h"

#include <vector>
#include <fstream>

#include "binary.h"

BNDFile::BNDFile(const byte* backingData):
	backingData(backingData) {}

std::string FindName(const byte* start, const byte* end) {
	const int extensionLength = 14;

	std::array<byte, extensionLength> wideExtension = {
		'.', 0, 'm', 0, 'a', 0, 't', 0, 'x', 0, 'm', 0, 'l', 0
	};

	byte* nameEndPos = const_cast<byte *>(end - extensionLength);

	for (nameEndPos; nameEndPos > start && memcmp(nameEndPos, wideExtension.data(), extensionLength) != 0; nameEndPos--);

	byte* nameStartPos = nameEndPos;

	for (nameStartPos; nameEndPos > start && *nameStartPos != '\\'; nameStartPos--);

	nameStartPos += 2;

	std::string result;
	// result.reserve((nameEndPos - nameStartPos) / 2);

	for (int i = 0; i < (nameEndPos - nameStartPos); i += 2) {
		if (nameStartPos[i]) {
			result.push_back(nameStartPos[i]);
		}
	}

	return result;
}

BNDFile* BNDFile::Parse(const byte* data, size_t dataLength) {
	int pos = 0;

	if (!AssertASCII(ExtractASCII<4>(data, pos), "BND4", "Magic value")) {
		return nullptr;
	}

	pos = 0xC;

	int fileCount = ExtractInt32(data, pos, false);

	pos = 0x28;

	pos = ExtractInt32(data, pos, false) + 8;

	BNDFile* resultFile = new BNDFile(data);

	const byte** offsets = new const byte*[fileCount];
	int count = 0;

	while (pos < dataLength) {
		if (AssertASCII(ExtractASCII<4>(data, pos), "MAB\0")) {
			offsets[count] = data + pos;

			count++;
		}
	}

	if (count != fileCount) {
		return nullptr;
	}

	for (int i = 0; i < count - 1; i++) {
		auto matName = FindName(offsets[i], offsets[i + 1]);

		resultFile->matbinFileOffsets[matName] = {offsets[i], offsets[i + 1]};
	}
	resultFile->matbinFileOffsets[FindName(offsets[count - 1], data + dataLength)] = MatbinFile(offsets[count - 1], data + dataLength);

	return resultFile;
}

MatbinFile BNDFile::GetMatbin(std::string name) {
	if (this->matbinFileOffsets.contains(name)) {
		return this->matbinFileOffsets[name];
	}

	return {nullptr, nullptr};
}
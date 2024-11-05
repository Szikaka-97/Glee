#include "matbin_file.h"

MatbinFile::MatbinFile():
	start(nullptr),
	end(nullptr) {}

MatbinFile::MatbinFile(const byte* start, const byte* end):
	start(start),
	end(end) {}


bool MatbinFile::FindProperty(const std::string& propertyName) {
	if (!this->IsValid()) {
		return false;
	}

	int offset = -1;

	if (propertyOffsets.contains(propertyName)) {
		offset = propertyOffsets[propertyName];
	}

	std::vector<byte> widePropertyName;
	widePropertyName.resize(propertyName.length() * 2);

	for (int i = 0; i < propertyName.length(); i++) {
		widePropertyName[2 * i] = propertyName[i];
		widePropertyName[2 * i + 1] = 0;
	}

	int windowWidth = widePropertyName.size();

	//Looking for a property
	for (byte* pos = const_cast<byte *>(this->end) - windowWidth; pos > this->start; pos--) {
		if (memcmp(pos, widePropertyName.data(), windowWidth) == 0) {
			offset = pos - this->start + windowWidth + 2;

			break;
		}
	}

	if (offset >= 0) {
		propertyOffsets[propertyName] = offset;
	}

	return offset >= 0;
}

inline bool MatbinFile::IsValid() {
	return this->start != nullptr && this->end != nullptr;
}

MatbinFile::operator bool() {
	return this->IsValid();
}
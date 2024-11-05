#pragma once

#include <string>
#include <vector>
#include <map>

#include "binary.h"

#include "spdlog/spdlog.h"

class MatbinFile {
private:
	const byte* start;
	const byte* end;

	std::map<std::string, int> propertyOffsets;
public:
	MatbinFile();
	MatbinFile(const byte* start, const byte* end);

	bool FindProperty(const std::string& propertyName);

	template<size_t Length>
	const std::array<float, Length> GetProperty(const std::string& propertyName) {
		static_assert(Length > 0 && Length <= 5);

		if (!propertyOffsets.contains(propertyName)) {
			FindProperty(propertyName);
		}

		if (propertyOffsets.contains(propertyName)) {
			int offset = propertyOffsets[propertyName];

			std::array<float, Length> result = {0};

			memcpy(result.data(), this->start + offset, sizeof(float) * Length);

			return result;
		}
		else {
			std::array<float, Length> result = {0};

			return result;
		}
	}

	template<size_t Length>
	void SetProperty(const std::string& propertyName, const std::array<float, Length>& values) {
		static_assert(Length > 0 && Length <= 5);

		if (!propertyOffsets.contains(propertyName)) {
			FindProperty(propertyName);
		}

		if (propertyOffsets.contains(propertyName)) {
			int offset = propertyOffsets[propertyName];

			memcpy(const_cast<byte *>(this->start) + offset, values.data(), sizeof(float) * Length);
		}
		else {
			spdlog::error("Couldn't find property: " + propertyName);
		}
	}

	byte* GetStart() { return const_cast<byte *>(this->start); }
	byte* GetEnd() { return const_cast<byte *>(this->end); }

	bool IsValid();
	operator bool();
};
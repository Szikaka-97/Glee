#pragma once

#include <string>
#include <filesystem>

namespace StringUtils {
	std::string Trim(const std::string& s);

	template<typename CharType>
	requires (
		std::same_as<CharType, char>
		||
		std::same_as<CharType, wchar_t>
	)
	int Length(CharType* p) {
		int length = 0;

		while (*p) {
			p++;
			length++;
		}

		return length;
	}
}

namespace PathUtils {
	std::filesystem::path StemWindows(const std::filesystem::path& p);
}
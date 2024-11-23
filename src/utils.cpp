#include "utils.h"

std::string StringUtils::Trim(const std::string& s) {
	return s.substr(s.find_first_not_of(' '), s.find_last_not_of(' '));
}

std::filesystem::path PathUtils::StemWindows(const std::filesystem::path& p) {
	std::string s = p.string();

	for (char& c : s) {
		if (c == '\\') {
			c = '/';
		}
	}

	return std::filesystem::path(s).stem();
}
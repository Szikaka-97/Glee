#include "logging.h"

#include <fstream>
#include <regex>

#include "spdlog/spdlog.h"
#include "spdlog/sinks/basic_file_sink.h"

fs::path GetLatestLogger(const fs::path& path) {
	fs::path logFolderPath = path / "logs";

	if (!fs::exists(logFolderPath)) {
		fs::create_directory(logFolderPath);
	}

	fs::path latest;

	for (const auto& dirFile : fs::directory_iterator(logFolderPath)) {
		if (!fs::exists(latest) || fs::last_write_time(dirFile.path()) > fs::last_write_time(latest)) {
			latest = dirFile;
		}
	}

	return latest;
}

// https://stackoverflow.com/a/59343785 <3
int GetFileCount(const fs::path& path) {
	auto dirIter = std::filesystem::directory_iterator(path);

	int fileCount = std::count_if(
		begin(dirIter),
		end(dirIter),
		[](auto& entry) { return entry.is_regular_file(); }
	);

	return fileCount;
}

std::tm GetFileCreationTime(const fs::path& path) {
	static const std::regex logTimePattern("\\[(\\d{4})-(\\d{2})-(\\d{2}) (\\d{2}):(\\d{2})");

	std::tm result;

	if (fs::exists(path)) {
		std::ifstream file(path);

		std::string firstLine;
		std::getline(file, firstLine);

		if (!firstLine.empty()) {
			std::smatch matchGroups;

			if (std::regex_search(firstLine, matchGroups, logTimePattern)) {
				result.tm_year = std::stoi(matchGroups[1].str());
				result.tm_mon = std::stoi(matchGroups[2].str());
				result.tm_mday = std::stoi(matchGroups[3].str());
				result.tm_hour = std::stoi(matchGroups[4].str());
				result.tm_min = std::stoi(matchGroups[5].str());
			}
		}
	}

	return result;
}

void Logging::InitLogger(const fs::path& path) {
#ifdef _WIN32
	if (GetFileCount(path) > 0) {
		auto latestFile = GetLatestLogger(path);

		const auto lastFileCreationTime = GetFileCreationTime(latestFile);

		std::string logFileName = "Glee-";
		logFileName += std::to_string(lastFileCreationTime.tm_year) + '-'
			+ std::to_string(lastFileCreationTime.tm_mon) + '-'
			+ std::to_string(lastFileCreationTime.tm_mday) + '_'
			+ std::to_string(lastFileCreationTime.tm_hour) + '-'
			+ std::format("{:02}", lastFileCreationTime.tm_min) + ".log";
		
		fs::rename(latestFile, path / "logs" / logFileName);
	}

	fs::path latestLogFilePath = path / "logs" / "Glee-latest.log";

	auto mainLog = spdlog::basic_logger_mt("main", latestLogFilePath.string());

	spdlog::set_default_logger(mainLog);
#endif
	spdlog::set_level(spdlog::level::trace);
	spdlog::flush_on(spdlog::level::trace);

	spdlog::info("Logger Initialized");
}
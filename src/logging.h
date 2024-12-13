#pragma once

#include <filesystem>
#include <chrono>

namespace fs = std::filesystem;

namespace Logging {
	void InitLogger(const fs::path& path);
}
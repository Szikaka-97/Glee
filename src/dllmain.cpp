#include <windows.h>

#include <fstream>
#include <filesystem>
#include <chrono>
#include <thread>

#include "spdlog/spdlog.h"
#include "spdlog/sinks/basic_file_sink.h"

namespace fs = std::filesystem;

void InitLogger(fs::path path) {
	const auto t = std::time(0);
	const auto now = std::localtime(&t);

	std::string logFileName = "Glee-";
	logFileName += std::to_string(now->tm_year - 100) + '-'
		+ std::to_string(now->tm_mon + 1) + '-'
		+ std::to_string(now->tm_mday) + '-'
		+ std::to_string(now->tm_hour) + '-'
		+ std::to_string(now->tm_min) + ".log";

	fs::path logFilePath = path / "logs" / logFileName;

	auto mainLog = spdlog::basic_logger_mt("main", logFilePath.string());

	spdlog::set_level(spdlog::level::trace);
	spdlog::flush_on(spdlog::level::info);

	spdlog::set_default_logger(mainLog);
}

void Start(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
	wchar_t dll_filename[MAX_PATH] = {0};
	GetModuleFileNameW(hinstDLL, dll_filename, MAX_PATH);
	auto folder = fs::path(dll_filename).parent_path();

	InitLogger(folder);

	spdlog::info("Initializing Glee");
}

void Dispose() {
	spdlog::info("Exiting Glee");
}

bool WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
	switch(fdwReason) { 
		case DLL_PROCESS_ATTACH:
			Start(hinstDLL, fdwReason, lpvReserved);

			break;

		case DLL_PROCESS_DETACH:
			Dispose();

			break;
	}
	return true;
}

#ifdef _WIN32
	#include <windows.h>

	#define DLL_RETURN bool WINAPI
#else
	#include <unistd.h>

	#define HINSTANCE int
	#define DWORD int
	#define LPVOID int
	#define DLL_RETURN bool
	#define DLL_PROCESS_ATTACH 0
	#define DLL_PROCESS_DETACH 1
#endif

#include <fstream>
#include <filesystem>
#include <chrono>
#include <thread>

#include "spdlog/spdlog.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "pugixml.hpp"

#include "dcx_file.h"
#include "matbin_file.h"
#include "bnd_file.h"

namespace fs = std::filesystem;

fs::path GetLibraryDir(HINSTANCE instanceID) {
	fs::path result;

#ifdef _WIN32
    wchar_t path[MAX_PATH] = { 0 };
    GetModuleFileNameW(instanceID, path, MAX_PATH);
    result =  path;
#else
    char path[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", path, PATH_MAX);
    result = std::string(path, (count > 0) ? count : 0);
#endif

	return result.parent_path();
}

fs::path pluginDir;

void InitLogger(fs::path path) {
#ifdef _WIN32
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
#endif
}

void LoadXMLs() {
	const auto sourceDCXpath = pluginDir / "assets" / "allmaterial.matbinbnd.dcx";

	spdlog::info("Starting XML modding");

	auto file = DCXFile::ReadFile(sourceDCXpath);

	if (!file) {
		spdlog::error("Couldn't read the source material file");

		return;
	}

	size_t actualDecompressedSize = 0;
	byte* outFileBuffer = file->Decompress(actualDecompressedSize);

	auto bnd = BNDFile::Parse(outFileBuffer, actualDecompressedSize);

	if (!bnd) {
		spdlog::error("Not good - BND");

		return;
	}

	MaterialMod mod;

	auto xmlSourceDir = pluginDir / "recolors";

	if (fs::exists(xmlSourceDir) && fs::is_directory(xmlSourceDir)) {
		for (const auto& file : fs::directory_iterator(xmlSourceDir)) {
			const auto& filePath = file.path();

			if (filePath.extension() == ".xml") {
				pugi::xml_document modDoc;

				pugi::xml_parse_result result = modDoc.load_file(filePath.c_str());
				if (!result) {
					spdlog::error(std::format("Failed to load the XML at path {}", filePath.string()));

					continue;
				}
				else {
					spdlog::info(std::format("Loaded the XML at path {}", filePath.string()));
				}

				mod.AddMod(modDoc);
			}
		}
	}
	else {
		if (!fs::exists(xmlSourceDir)) {
			spdlog::error("Directory {} doesn't exist!", xmlSourceDir.string());
		}
		else {
			spdlog::error("{} isn't a directory!", xmlSourceDir.string());
		}
	}

	bnd->ApplyMod(mod);

	auto newDcx = DCXFile::Pack(outFileBuffer, actualDecompressedSize);

	const auto newDCXFilePath = pluginDir / "material" / "allmaterial.matbinbnd.dcx";

	newDcx->WriteFile(newDCXFilePath);

	delete bnd;
	delete file;

	spdlog::info("Finished modding");
}

void Start(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
	pluginDir = GetLibraryDir(hinstDLL);

	InitLogger(pluginDir);

	spdlog::info("Initializing Glee");

	LoadXMLs();
}

void Dispose() {
	spdlog::info("Exiting Glee");

	const auto createdDCXpath = pluginDir / "material" / "allmaterial.matbinbnd.dcx";

	if (fs::exists(createdDCXpath)) {
		if (fs::remove(createdDCXpath)) {
			spdlog::info("Cleaned up files");
		}
		else {
			spdlog::error("Couldn't clean up files");
		}
	}
}

DLL_RETURN DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
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

#ifndef _WIN32
	int main() {
		if (DllMain(0, DLL_PROCESS_ATTACH, 0)) {
			return 0;
		}
		
		return 1;
	}
#endif

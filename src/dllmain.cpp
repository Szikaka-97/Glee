#include <windows.h>

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

fs::path pluginPath;

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

void TestChange() {
	const auto sourceDCXpath = pluginPath / "assets" / "allmaterial.matbinbnd.dcx";

	spdlog::info("Starting modding");

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

	auto fabricMat = bnd->GetMatbin("P[BD_M_1710]_Fabric");

	if (!fabricMat) {
		spdlog::error("Not good - MATBIN");

		return;
	}

	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file((pluginPath / "xml" / "examplemod.xml").c_str());
	if (!result) {
		std::cout << "Failed to load the XML\n";

		return;
	}

	MaterialMod mod;

	mod.AddMod(doc);

	bnd->ApplyMod(mod);

	auto newDcx = DCXFile::Pack(outFileBuffer, actualDecompressedSize);

	const auto newDCXFilePath = pluginPath / "material" / "allmaterial.matbinbnd.dcx";

	newDcx->WriteFile(newDCXFilePath);

	spdlog::info("Finished modding");

	delete outFileBuffer;
	delete bnd;
	delete file;
}

void LoadXMLs() {
	const auto sourceDCXpath = pluginPath / "assets" / "allmaterial.matbinbnd.dcx";

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

	auto xmlSourceDir = pluginPath / "recolors";

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

	const auto newDCXFilePath = pluginPath / "material" / "allmaterial.matbinbnd.dcx";

	newDcx->WriteFile(newDCXFilePath);

	delete bnd;
	delete file;

	spdlog::info("Finished modding");
}

void Start(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
	wchar_t dll_filename[MAX_PATH] = {0};
	GetModuleFileNameW(hinstDLL, dll_filename, MAX_PATH);
	auto folder = fs::path(dll_filename).parent_path();

	pluginPath = folder;

	InitLogger(folder);

	spdlog::info("Initializing Glee");

	LoadXMLs();
}

void Dispose() {
	spdlog::info("Exiting Glee");

	const auto createdDCXpath = pluginPath / "material" / "allmaterial.matbinbnd.dcx";

	if (fs::exists(createdDCXpath)) {
		if (fs::remove(createdDCXpath)) {
			spdlog::info("Cleaned up files");
		}
		else {
			spdlog::error("Couldn't clean up files");
		}
	}
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

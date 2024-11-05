#include <windows.h>

#include <fstream>
#include <filesystem>
#include <chrono>
#include <thread>

#include "spdlog/spdlog.h"
#include "spdlog/sinks/basic_file_sink.h"

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

	std::array<float, 5> gotValues = fabricMat.GetProperty<5>("C_DetailBlend__Rich__snp_0_color_0");

	gotValues[0] = 1;
	gotValues[1] = 0;
	gotValues[2] = 0;

	fabricMat.SetProperty("C_DetailBlend__Rich__snp_0_color_0", gotValues);

	auto newDcx = DCXFile::Pack(outFileBuffer, actualDecompressedSize);

	const auto newDCXFile = pluginPath / "material" / "allmaterial.matbinbnd.dcx";

	newDcx->WriteFile(newDCXFile);

	spdlog::info("Finished modding");

	delete outFileBuffer;
	delete bnd;
	delete file;
}

void Start(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
	wchar_t dll_filename[MAX_PATH] = {0};
	GetModuleFileNameW(hinstDLL, dll_filename, MAX_PATH);
	auto folder = fs::path(dll_filename).parent_path();

	pluginPath = folder;

	InitLogger(folder);

	spdlog::info("Initializing Glee");

	TestChange();
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

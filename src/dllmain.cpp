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
#include "pugixml.hpp"

#include "dcx_file.h"
#include "matbin_file.h"
#include "bnd_file.h"
#include "logging.h"

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

void LoadXMLs() {
	const auto sourceDCXpath = pluginDir / "assets" / "allmaterial.matbinbnd.dcx";

	spdlog::info("Starting XML modding");

	auto sourceMatFile = DCXFile::ReadFile(sourceDCXpath);

	if (!sourceMatFile) {
		spdlog::error("Couldn't read the source material file");

		return;
	}

	auto bnd = BNDFile::Unpack(sourceMatFile);

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
					spdlog::error("Failed to load the XML at path {}", filePath.string());

					continue;
				}
				else {
					spdlog::info("Loaded the XML at path {}", filePath.string());
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

	auto destMatFile = bnd->Pack();

	const auto newDCXFilePath = pluginDir / "material" / "allmaterial.matbinbnd.dcx";

	destMatFile->WriteFile(newDCXFilePath);

	delete bnd;
	delete sourceMatFile;

	spdlog::info("Finished modding");
}

void TestBNDWrite() {
	// Test if the BND is generated correctly by comparing it with the original
	// As of now I can't be bothered with replicating the quircks of the BND4 format exactly, so our criteria is "functionally equal"

	const auto sourceDCXpath = pluginDir / "assets" / "allmaterial.matbinbnd.dcx";

	spdlog::info("Testing BND writing");

	auto sourceMatFile = DCXFile::ReadFile(sourceDCXpath);

	if (!sourceMatFile) {
		spdlog::error("Couldn't read the source material file");

		return;
	}

	auto bnd = BNDFile::Unpack(sourceMatFile);

	if (!bnd) {
		spdlog::error("Not good - BND");

		return;
	}

	const auto sourceBNDpath = pluginDir / "test" / "src.bnd";

	std::ofstream file(sourceBNDpath, std::ios::binary);

	file.write((char *) bnd->GetData(), bnd->GetSize());

// Make a change

	pugi::xml_document modDoc;

	modDoc.load_file((pluginDir / "recolors" / "examplemod.xml").c_str());

	MaterialMod mod{};
	mod.AddMod(modDoc);

	bnd->ApplyMod(mod);

	const auto destBNDpath = pluginDir / "test" / "out.bnd";

	bnd->Write(destBNDpath);

// Test if the written thing is correct

	size_t bndFileSize = fs::file_size(pluginDir / "test" / "out.bnd");
	byte* newBNDData = new byte[bndFileSize];
	std::ifstream newBndFileStream(pluginDir / "test" / "out.bnd");
	newBndFileStream.read((char *) newBNDData, bndFileSize);
	auto newBND = BNDFile::Parse(newBNDData, bndFileSize);

	if (bnd->GetSize() != newBND->GetSize()) {
		spdlog::error("BND: Sizes don't match (this is expected, though) {} != {}", bnd->GetSize(), newBND->GetSize());
	}

	int failCount = 0;
	for (const std::string* path : bnd->GetAllMatbinPaths(false)) {
		auto matbin = newBND->GetMatbin(*path);

		if (!matbin) {
			spdlog::error("BND: Can't find matbin {}", *path);

			failCount++;

			continue;
		}

		auto reference = bnd->GetMatbin(*path);

		bool errored = false;

		if (matbin->GetLength() != reference->GetLength()) {
			spdlog::error("BND: Incorrect matbin {}, lengths don't match: {} != {}", *path, matbin->GetLength(), reference->GetLength());

			errored = true;
		}

		if (matbin->ParamCount() != reference->ParamCount()) {
			spdlog::error("BND: Incorrect matbin {}, param counts don't match: {} != {}", *path, matbin->ParamCount(), reference->ParamCount());

			errored = true;
		}
		if (matbin->SamplerCount() != reference->SamplerCount()) {
			spdlog::error("BND: Incorrect matbin {}, sampler counts don't match: {} != {}", *path, matbin->SamplerCount(), reference->SamplerCount());

			errored = true;
		}
		
		if (!errored) {
			if (memcmp(matbin->GetStart(), reference->GetStart(), matbin->GetLength()) != 0) {
				spdlog::error("BND: Incorrect matbin {}, parameter values don't match", *path);

				errored = true;
			}
		}

		failCount += errored;
	}
	if (failCount > 0) {
		spdlog::error("Final error count: {}/{}", failCount, bnd->GetAllMatbinPaths().size());
	}
	else {
		spdlog::info("All correct");
	}
}

void Start(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
	pluginDir = GetLibraryDir(hinstDLL);

	Logging::InitLogger(pluginDir);

	spdlog::info("Initialized Glee");

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

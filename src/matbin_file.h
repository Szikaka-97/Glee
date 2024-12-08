#pragma once

#include <string>
#include <vector>
#include <map>
#include <concepts>

#include "binary.h"
#include "material_change.h"
#include "utils.h"

#include "spdlog/spdlog.h"

template<typename T, size_t Length>
concept ParamValue = (
	(std::is_same<float, T>::value && Length > 0 && Length <= 5)
	||
	(std::is_same<int, T>::value && Length > 0 && Length <= 2)
	||
	(std::is_same<bool, T>::value && Length == 1)
);


class MatbinFile {
private:
	struct ParamInfo {
		std::string name;
		void* valuePtr;
		int key;
		int type;

		static constexpr size_t GetByteSize() {
			return (
				8 + 8 + 4 + 4 + 0x10
			);
		}
	};

	template<typename T, size_t Length = 1>
	requires ParamValue<T, Length>
	struct Param {
		short infoIndex;

		static constexpr byte ParamTypeMask() {
			return {
				std::is_same<bool, T>::value
				|
				std::is_same<int, T>::value << Length
				|
				std::is_same<float, T>::value << (Length + 2)
			};
		}

		Param(short infoIndex):
		infoIndex(infoIndex) {}

		byte GetMask() {
			return Param<T, Length>::ParamTypeMask();
		}
	};

	struct TextureParam {
		byte* header;
		const std::string name;
		std::string path;
		const uint key;
		const std::array<float, 2> unk;

		TextureParam(byte* header, const std::string& name, std::string path, uint key, const float unk1, const float unk2):
		header(header),
		name(name),
		path(path),
		key(key),
		unk({unk1, unk2}) { }

		static constexpr size_t GetByteSize() {
			return (
				8 + 8 + 4 + 4 * 2 + 0x14
			);
		}
	};

	byte* start;
	byte* end;
	byte* dumbDataEnd;

	std::string shaderPath;
	std::string sourcePath;
	unsigned int key;
	bool relocated;

	std::vector<ParamInfo> params;
	std::vector<Param<bool>> boolParams;
	std::vector<Param<int, 1>> int1Params;
	std::vector<Param<int, 2>> int2Params;
	std::vector<Param<float, 1>> float1Params;
	std::vector<Param<float, 2>> float2Params;
	std::vector<Param<float, 3>> float3Params;
	std::vector<Param<float, 4>> float4Params;
	std::vector<Param<float, 5>> float5Params;
	std::vector<TextureParam> samplers;

	std::map<std::string, int> propertyOffsets;

	void ReadParam(BufferView& data);
	void ReadSampler(BufferView& data);

	void Relocate(byte* newStart, size_t newLength);

	template<typename T, size_t Length>
	requires ParamValue<T, Length>
	T* GetParamValue(const Param<T, Length>* param) {
		return (T *) this->params[param->infoIndex].valuePtr;
	}

	void GetParam(Param<bool, 1>*& param, const std::string& propertyName);
	void GetParam(Param<int, 1>*& param, const std::string& propertyName);
	void GetParam(Param<int, 2>*& param, const std::string& propertyName);
	void GetParam(Param<float, 1>*& param, const std::string& propertyName);
	void GetParam(Param<float, 2>*& param, const std::string& propertyName);
	void GetParam(Param<float, 3>*& param, const std::string& propertyName);
	void GetParam(Param<float, 4>*& param, const std::string& propertyName);
	void GetParam(Param<float, 5>*& param, const std::string& propertyName);
	void GetSampler(TextureParam*& param, const std::string& propertyName);
public:
	MatbinFile(byte* start, size_t length);

	template<typename T, size_t Length>
	requires ParamValue<T, Length>
	bool HasProperty(const std::string& propertyName) {
		Param<T, Length>* param = nullptr;

		GetParam(param, propertyName);

		return param != nullptr;
	}

	template<typename T, size_t Length>
	requires ParamValue<T, Length>
	std::array<T, Length> GetPropertyValues(const std::string& propertyName) {
		Param<T, Length>* param = nullptr;

		GetParam(param, propertyName);

		if (!param) {
			throw std::out_of_range(std::format("No param named {}", propertyName));
		}

		std::array<T, Length> result{0};

		memcpy(result.data(), GetParamValue(param), sizeof(T) * Length);

		return result;
	}

	template<typename T, size_t Length>
	requires ParamValue<T, Length>
	void SetPropertyValues(const std::string& propertyName, const std::array<T, Length>& values) {
		Param<T, Length>* param = nullptr;

		GetParam(param, propertyName);

		if (!param) {
			throw std::out_of_range(std::format("No param named {}", propertyName));
		}

		memcpy(const_cast<T *>(GetParamValue(param)), values.data(), sizeof(T) * Length);

		spdlog::info("Value: {} {} {}", GetParamValue(param)[0], GetParamValue(param)[1], GetParamValue(param)[2]);
	}

	void ApplyMod(const MaterialChange& change);

	inline byte* GetStart() { return this->start; }
	inline byte* GetEnd() { return this->end; }

	inline size_t GetLength() { return this->end - this->start; }

	int ParamCount();
	int SamplerCount();

	inline bool WasRelocated() { return this->relocated; }

	inline bool IsValid() { return this->start != nullptr && this->end != nullptr; }
	operator bool();
};

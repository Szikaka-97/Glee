#pragma once

#include <string>
#include <vector>
#include <map>
#include <concepts>

#include "binary.h"

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
	template<typename T, size_t Length = 1>
	requires ParamValue<T, Length>
	struct Param {
		const std::string name;
		const int key;
		const T* value;

		static constexpr byte ParamTypeMask() {
			return {
				std::is_same<bool, T>::value
				|
				std::is_same<int, T>::value << Length
				|
				std::is_same<float, T>::value << (Length + 2)
			};
		}

		Param(const std::string& name, int key, const T* value):
		name(name),
		key(key),
		value(value) {}

		byte GetMask() {
			return Param<T, Length>::ParamTypeMask();
		}
	};

	const byte* start;
	const byte* end;

	std::string shaderPath;
	std::string sourcePath;
	unsigned int key;

	std::vector<Param<bool>> boolParams;
	std::vector<Param<int, 1>> int1Params;
	std::vector<Param<int, 2>> int2Params;
	std::vector<Param<float, 1>> float1Params;
	std::vector<Param<float, 2>> float2Params;
	std::vector<Param<float, 3>> float3Params;
	std::vector<Param<float, 4>> float4Params;
	std::vector<Param<float, 5>> float5Params;

	std::map<std::string, int> propertyOffsets;

	void ReadParam(BufferView& data);

	void GetParam(Param<bool, 1>*& param, const std::string& propertyName);
	void GetParam(Param<int, 1>*& param, const std::string& propertyName);
	void GetParam(Param<int, 2>*& param, const std::string& propertyName);
	void GetParam(Param<float, 1>*& param, const std::string& propertyName);
	void GetParam(Param<float, 2>*& param, const std::string& propertyName);
	void GetParam(Param<float, 3>*& param, const std::string& propertyName);
	void GetParam(Param<float, 4>*& param, const std::string& propertyName);
	void GetParam(Param<float, 5>*& param, const std::string& propertyName);
public:
	MatbinFile(const byte* start, size_t length);

	template<typename T, size_t Length>
	requires ParamValue<T, Length>
	std::array<T, Length> GetPropertyValues(const std::string& propertyName) {
		Param<T, Length>* param = nullptr;

		GetParam(param, propertyName);

		if (!param) {
			throw std::out_of_range(std::format("No param named {}", propertyName));
		}

		std::array<T, Length> result{0};

		memcpy(result.data(), param->value, sizeof(T) * Length);

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

		memcpy(const_cast<T *>(param->value), values.data(), sizeof(T) * Length);
	}

	byte* GetStart() { return const_cast<byte *>(this->start); }
	byte* GetEnd() { return const_cast<byte *>(this->end); }

	bool IsValid();
	operator bool();
};

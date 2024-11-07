#include "matbin_file.h"

#include <exception>

// The stuff you'll do to avoid writing code...
#define SEARCH_PARAMS(arrayName) for (auto& p : arrayName) { if (p.name == propertyName) { param = &p; break; } };
	
enum ParamType {
	Bool = 0,
	Int = 4,
	Int2 = 5,
	Float = 8,
	Float2 = 9,
	Float3 = 10,
	Float4 = 11,
	Float5 = 12,
};

void MatbinFile::ReadParam(BufferView& data) {
	std::string paramName = data.ReadOffsetUTF16();

	uint64_t valueOffset = data.ReadInt64();
	int key = data.ReadInt32();
	int type = data.ReadInt32();

	for (int i = 0; i < 0x10; i++) {
		data.AssertByte(0, "MAB Padding");
	}

	auto currentOffset = data.GetOffset();

	data.SetOffset(valueOffset);

	switch (type) {
	case ParamType::Bool:
	{
		auto values = (bool *) data.GetPos();
		data.Skip<bool>();

		auto param = MatbinFile::Param<bool, 1>(paramName, key, values);

		this->boolParams.push_back(param);

		break;
	}
	case ParamType::Int:
	{
		auto values = (int *) data.GetPos();
		data.Skip<int>();

		auto param = MatbinFile::Param<int, 1>(paramName, key, values);

		this->int1Params.push_back(param);

		break;
	}
	case ParamType::Int2:
	{
		auto values = (int *) data.GetPos();
		data.Skip<int, 2>();

		auto param = MatbinFile::Param<int, 2>(paramName, key, values);

		this->int2Params.push_back(param);

		break;
	}
	case ParamType::Float:
	{
		auto values = (float *) data.GetPos();
		data.Skip<float, 1>();

		auto param = MatbinFile::Param<float, 1>(paramName, key, values);

		this->float1Params.push_back(param);

		break;
	}
	case ParamType::Float2:
	{
		auto values = (float *) data.GetPos();
		data.Skip<float, 2>();

		auto param = MatbinFile::Param<float, 2>(paramName, key, values);

		this->float2Params.push_back(param);

		break;
	}
	case ParamType::Float3:
	{
		auto values = (float *) data.GetPos();
		data.Skip<float, 3>();

		auto param = MatbinFile::Param<float, 3>(paramName, key, values);

		this->float3Params.push_back(param);

		break;
	}
	case ParamType::Float4:
	{
		auto values = (float *) data.GetPos();
		data.Skip<float, 4>();

		auto param = MatbinFile::Param<float, 4>(paramName, key, values);

		this->float4Params.push_back(param);

		break;
	}
	case ParamType::Float5:
	{
		auto values = (float *) data.GetPos();
		data.Skip<float, 5>();

		auto param = MatbinFile::Param<float, 5>(paramName, key, values);

		this->float5Params.push_back(param);

		break;
	}
	}

	data.SetOffset(currentOffset);
}

MatbinFile::MatbinFile(const byte* start, size_t length):
start(start),
end(start + length) {
	BufferView dataView(start, end, false);

	dataView.AssertASCII("MAB", 4, "MAB Magic Value");
	dataView.AssertInt32(2, "MAB Version");

	this->shaderPath = dataView.ReadOffsetUTF16();
	
	this->sourcePath = dataView.ReadOffsetUTF16();

	this->key = dataView.ReadInt32();

	int paramCount = dataView.ReadInt32();
	int samplerCount = dataView.ReadInt32();
	
	for (int i = 0; i < 0x14; i++) {
		dataView.AssertByte(0, "MAB Padding");
	}

	for (int i = 0; i < paramCount; i++) {
		ReadParam(dataView);
	}
}

inline bool MatbinFile::IsValid() {
	return this->start != nullptr && this->end != nullptr;
}

MatbinFile::operator bool() {
	return this->IsValid();
}

template<typename T, size_t Length>
constexpr int GetByteLength(const std::array<T, Length>&) {
	return sizeof(T) * Length;
}
void MatbinFile::GetParam(Param<bool, 1>*& param, const std::string& propertyName) { SEARCH_PARAMS(this->boolParams) }
void MatbinFile::GetParam(Param<int, 1>*& param, const std::string& propertyName) { SEARCH_PARAMS(this->int1Params) }
void MatbinFile::GetParam(Param<int, 2>*& param, const std::string& propertyName) { SEARCH_PARAMS(this->int2Params) }
void MatbinFile::GetParam(Param<float, 1>*& param, const std::string& propertyName) { SEARCH_PARAMS(this->float1Params) }
void MatbinFile::GetParam(Param<float, 2>*& param, const std::string& propertyName) { SEARCH_PARAMS(this->float2Params) }
void MatbinFile::GetParam(Param<float, 3>*& param, const std::string& propertyName) { SEARCH_PARAMS(this->float3Params) }
void MatbinFile::GetParam(Param<float, 4>*& param, const std::string& propertyName) { SEARCH_PARAMS(this->float4Params) }
void MatbinFile::GetParam(Param<float, 5>*& param, const std::string& propertyName) { SEARCH_PARAMS(this->float5Params) }
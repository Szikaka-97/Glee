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

	if (data.GetPos() > this->dumbDataEnd) {
		this->dumbDataEnd = data.GetPos();
	}

	data.SetOffset(currentOffset);
}

void MatbinFile::ReadSampler(BufferView& data) {
	byte* headerPos = data.GetPos();

	std::string samplerName = data.ReadOffsetUTF16();
	std::string path = data.ReadOffsetUTF16();
	uint key = data.ReadInt32();
	auto unk = data.ReadFloatArray<2>();

	for (int i = 0; i < 0x14; i++) {
		data.AssertByte(0, "MAB Padding");
	}

	this->samplers.push_back(TextureParam(
		headerPos,
		samplerName,
		path,
		key,
		unk[0], unk[1]
	));
}

MatbinFile::MatbinFile(byte* start, size_t length):
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

	for (int i = 0; i < samplerCount; i++) {
		ReadSampler(dataView);

		// spdlog::info(std::format("Sampler: {}, texture path: {}", this->samplers.back().name, this->samplers.back().GetPath()));
	}
}

template<typename T, size_t Length>
requires ParamValue<T, Length>
bool ApplyPropertyChange(MatbinFile* mat, const PropertyChange& propChange) {
	if (mat->HasProperty<T, Length>(propChange.target)) {
		spdlog::info(" Changed property {}", propChange.target);

		std::array<T, Length> values = mat->GetPropertyValues<T, Length>(propChange.target);

		for (int i = 0; i < Length; i++) {
			if (propChange.values[i].enabled) {
				values[i] = (T) propChange.values[i].value;
			}
		}

		mat->SetPropertyValues(propChange.target, values);

		return true;
	}

	return false;
}

void MatbinFile::ApplyMod(const MaterialChange& change) {
	for (const auto& propChange : change.GetPropertyChanges()) {
		bool result = (
			ApplyPropertyChange<bool, 1>(this, propChange)
			||
			ApplyPropertyChange<int, 1>(this, propChange)
			||
			ApplyPropertyChange<int, 2>(this, propChange)
			||
			ApplyPropertyChange<float, 1>(this, propChange)
			||
			ApplyPropertyChange<float, 2>(this, propChange)
			||
			ApplyPropertyChange<float, 3>(this, propChange)
			||
			ApplyPropertyChange<float, 4>(this, propChange)
			||
			ApplyPropertyChange<float, 5>(this, propChange)
		);

		if (!result) {
			// ERROR - possible typo or shader mismatch

			spdlog::error("No change");
		}
	}

	int sizeChange = 0;

	for (const auto& texChange : change.GetTextureChanges()) {
		TextureParam* param;
		GetSampler(param, texChange.target);

		if (!param) {
			// ERROR - No param

			continue;
		}

		spdlog::info(" Changed sampler {}", texChange.target);

		sizeChange += (texChange.newPath.length() - param->path.length()) * 2;

		param->path = texChange.newPath;
	}

	spdlog::info("Size change: {}", sizeChange);

	if (sizeChange != 0) {
		size_t newLength = this->end - this->start + sizeChange;

		byte* newLocation = new byte[newLength];

		memcpy(newLocation, this->start, this->dumbDataEnd - this->start);

		TransferParams(newLocation, newLength);

		this->start = newLocation;
		this->end = newLocation + newLength;

		this->relocated = true;
	}
}

void MatbinFile::TransferParams(byte* newStart, size_t newLength) {
	for (auto& param : boolParams) {
		int offset = (byte *) param.value - newStart;

		param.value = (bool*) (newStart + offset);
	}
	for (auto& param : int1Params) {
		int offset = (byte *) param.value - newStart;

		param.value = (int*) (newStart + offset);
	}
	for (auto& param : int2Params) {
		int offset = (byte *) param.value - newStart;

		param.value = (int*) (newStart + offset);
	}
	for (auto& param : float1Params) {
		int offset = (byte *) param.value - newStart;

		param.value = (float*) (newStart + offset);
	}
	for (auto& param : float2Params) {
		int offset = (byte *) param.value - newStart;

		param.value = (float*) (newStart + offset);
	}
	for (auto& param : float3Params) {
		int offset = (byte *) param.value - newStart;

		param.value = (float*) (newStart + offset);
	}
	for (auto& param : float4Params) {
		int offset = (byte *) param.value - newStart;

		param.value = (float*) (newStart + offset);
	}
	for (auto& param : float5Params) {
		int offset = (byte *) param.value - newStart;

		param.value = (float*) (newStart + offset);
	}

	// Samplers are a bit more complicated
	int currentOffset = this->dumbDataEnd - this->start;
	BufferView dataView(newStart, newLength);

	for (auto& sampler : samplers) {
		int offset = sampler.header - this->start;
		sampler.header = newStart + offset;

		// Dirty solution, I'll think of something more clever later
		((uint64_t *) sampler.header)[1] = currentOffset;

		dataView.SetOffset(currentOffset);

		dataView.WriteUTF16(sampler.name);
		dataView.WriteUTF16(sampler.path);
	}
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
void MatbinFile::GetSampler(TextureParam*& param, const std::string& propertyName) { SEARCH_PARAMS(this->samplers) }
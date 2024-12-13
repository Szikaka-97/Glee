#include "matbin_file.h"

#include <exception>

// The stuff you'll do to avoid writing code...
#define SEARCH_PARAMS(arrayName) for (auto& p : arrayName) { if (this->params[p.infoIndex].name == propertyName) { param = &p; break; } };

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

	ParamInfo info{paramName, this->start + valueOffset, key, type};

	int infoIndex = this->params.size();
	this->params.push_back(info);

	switch (type) {
	case ParamType::Bool:
	{
		auto param = MatbinFile::Param<bool, 1>(infoIndex);

		this->boolParams.push_back(param);

		break;
	}
	case ParamType::Int:
	{
		auto param = MatbinFile::Param<int, 1>(infoIndex);

		this->int1Params.push_back(param);

		break;
	}
	case ParamType::Int2:
	{
		auto param = MatbinFile::Param<int, 2>(infoIndex);

		this->int2Params.push_back(param);

		break;
	}
	case ParamType::Float:
	{
		auto param = MatbinFile::Param<float, 1>(infoIndex);

		this->float1Params.push_back(param);

		break;
	}
	case ParamType::Float2:
	{
		auto param = MatbinFile::Param<float, 2>(infoIndex);

		this->float2Params.push_back(param);

		break;
	}
	case ParamType::Float3:
	{
		auto param = MatbinFile::Param<float, 3>(infoIndex);

		this->float3Params.push_back(param);

		break;
	}
	case ParamType::Float4:
	{
		auto param = MatbinFile::Param<float, 4>(infoIndex);

		this->float4Params.push_back(param);

		break;
	}
	case ParamType::Float5:
	{
		auto param = MatbinFile::Param<float, 5>(infoIndex);

		this->float5Params.push_back(param);

		break;
	}
	}
}

void MatbinFile::ReadSampler(BufferView& data) {
	byte* headerPos = data.GetPos();

	std::string samplerName = data.ReadOffsetUTF16();
	std::string path = data.ReadOffsetUTF16();
	unsigned int key = data.ReadInt32();
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
end(start + length),
relocated(false) {
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
			spdlog::error("Couldn't find param named {}, continuing to the next one", propChange.target);
		}
	}

	int sizeChange = 0;

	for (const auto& texChange : change.GetTextureChanges()) {
		TextureParam* param;
		GetSampler(param, texChange.target);

		if (!param) {
			spdlog::error("Couldn't find sampler named {}, continuing to the next one", texChange.target);

			continue;
		}

		spdlog::info(" Changed texture path {}", texChange.target);

		sizeChange += (texChange.newPath.length() - param->path.length()) * 2;

		param->path = texChange.newPath;
	}

	spdlog::info("Size change: {}", sizeChange);

	if (sizeChange != 0) {
		size_t newLength = this->end - this->start + sizeChange;

		byte* newLocation = new byte[newLength];

		Relocate(newLocation, newLength);
	}
}

void MatbinFile::Relocate(byte* newStart, size_t newLength) {
	BufferView dataView(newStart, newLength, false);

	dataView.WriteASCII("MAB", true);
	dataView.WriteInt32(2);

	int shaderOffset = newLength - (this->shaderPath.size() + this->sourcePath.size() + 2) * 2;
	dataView.WriteInt64(shaderOffset);
	dataView.SetOffset(shaderOffset);
	dataView.WriteUTF16(this->shaderPath);
	dataView.SetOffset(16);

	int sourceOffset = newLength - (this->sourcePath.size() + 1) * 2;
	dataView.WriteInt64(sourceOffset);
	dataView.SetOffset(sourceOffset);
	dataView.WriteUTF16(this->sourcePath);
	dataView.SetOffset(24);

	dataView.WriteInt32(this->key);
	dataView.WriteInt32(this->params.size());
	dataView.WriteInt32(this->SamplerCount());

	dataView.Write(std::array<byte, 0x14>({0}));

	int baseOffset = dataView.GetOffset() + this->params.size() * ParamInfo::GetByteSize() + this->samplers.size() * TextureParam::GetByteSize();
	int nameOffset = 0;

	for (const auto& paramInfo : this->params) {
		dataView.WriteInt64(baseOffset + nameOffset);
		dataView.WriteInt64((uint64_t) ((byte *) paramInfo.valuePtr - this->start));
		dataView.WriteInt32(paramInfo.key);
		dataView.WriteInt32(paramInfo.type);

		dataView.Write<0x10>({0});
		
		nameOffset += (paramInfo.name.size() * 2 + 2);

		int additionalOffset = 0;
		switch (paramInfo.type) {
			case ParamType::Bool:
				additionalOffset += 1;
				break;
			case ParamType::Float5:
				additionalOffset += 4;
			case ParamType::Float4:
				additionalOffset += 4;
			case ParamType::Float3:
				additionalOffset += 4;
			case ParamType::Int2:
			case ParamType::Float2:
				additionalOffset += 4;
			case ParamType::Int:
			case ParamType::Float:
				additionalOffset += 4;
				break;
		}

		// This type is actually a float5, with the last 2 values being useless
		// We still have to account for their size though
		if (paramInfo.type == ParamType::Float3) {
			additionalOffset += 8;
		}

		nameOffset += additionalOffset;
	}

	for (const auto& sampler : this->samplers) {
		dataView.WriteInt64(baseOffset + nameOffset);

		nameOffset += sampler.name.size() * 2 + 2;

		dataView.WriteInt64(baseOffset + nameOffset);

		nameOffset += sampler.path.size() * 2 + 2;

		dataView.WriteInt32(sampler.key);
		dataView.WriteInt32(sampler.unk[0]);
		dataView.WriteInt32(sampler.unk[1]);

		dataView.Write<0x14>({0});
	}

	for (auto& paramInfo : this->params) {
		dataView.WriteUTF16(paramInfo.name);
		
		void* newValuePtr = dataView.GetPos();

		if (paramInfo.type == ParamType::Bool) {
			dataView.Write<1>({* (bool *)paramInfo.valuePtr});
		}
		else if (paramInfo.type & ParamType::Float) {
			for (int i = 0; i < (paramInfo.type & 7) + 1; i++) {
				dataView.WriteFloat(* ((float *) paramInfo.valuePtr + i));
			}

			if (paramInfo.type == ParamType::Float3) {
				dataView.WriteFloat(1);
				dataView.WriteFloat(1);
			}
		}
		else if (paramInfo.type & ParamType::Int) {

			for (int i = 0; i < (paramInfo.type & 1) + 1; i++) {
				dataView.WriteInt32(* ((int *) paramInfo.valuePtr + i));
			}
		}

		paramInfo.valuePtr = newValuePtr;
	}

	for (const auto& sampler : this->samplers) {
		dataView.WriteUTF16(sampler.name);
		dataView.WriteUTF16(sampler.path);
	}

	this->start = newStart;
	this->end = newStart + newLength;

	this->relocated = true;
}

int MatbinFile::ParamCount() {
	return this->params.size();
}

int MatbinFile::SamplerCount() {
	return this->samplers.size();
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
void MatbinFile::GetSampler(TextureParam*& param, const std::string& propertyName) {
	for (auto& sampler : this->samplers) {
		if (sampler.name == propertyName) {
			param = &sampler;
			break;
		}
	};
}
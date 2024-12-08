#include "binary.h"

#include <format>
#include <fstream>

#include "spdlog/spdlog.h"

const std::string BufferView::ReadASCII(int length) {
	if (current + length > end) {
		throw std::out_of_range(
			std::format("Buffer overflow at ReadASCII offset= {} length= {}", this->current - this->start, length)
		);
	}

	std::string result((char*) current, length);

	current += length;

	return result;
}

byte BufferView::ReadByte() {
	const std::array<byte, sizeof(byte)> byteContainer = Read<sizeof(byte)>();

	return byteContainer[0];
}

bool BufferView::ReadBoolean() {
	return ReadByte() == 1;
}

int BufferView::ReadInt32() {
	const std::array<byte, sizeof(int)> intContainer = Read<sizeof(int)>();

	int result = 0;

	unsigned int b0 = intContainer[0];
	unsigned int b1 = intContainer[1];
	unsigned int b2 = intContainer[2];
	unsigned int b3 = intContainer[3];

	if (bigEndian) {
		result = (b0 << 24) | (b1 << 16) | (b2 << 8) | b3;
	}
	else {
		result = (b3 << 24) | (b2 << 16) | (b1 << 8) | b0;
	}

	return result;
}

uint64_t BufferView::ReadInt64() {
	const std::array<byte, sizeof(uint64_t)> intContainer = Read<sizeof(uint64_t)>();

	uint64_t result = 0;

	uint64_t b0 = intContainer[0];
	uint64_t b1 = intContainer[1];
	uint64_t b2 = intContainer[2];
	uint64_t b3 = intContainer[3];
	uint64_t b4 = intContainer[4];
	uint64_t b5 = intContainer[5];
	uint64_t b6 = intContainer[6];
	uint64_t b7 = intContainer[7];

	if (bigEndian) {
		result = (b0 << 56) | (b1 << 48) | (b2 << 40) | (b3 << 32) | (b4 << 24) | (b5 << 16) | (b6 << 8) | b7;
	}
	else {
		result = (b7 << 56) | (b6 << 48) | (b5 << 40) | (b4 << 32) | (b3 << 24) | (b2 << 16) | (b1 << 8) | b0;
	}

	return result;
}

float BufferView::ReadFloat() {
	unsigned int container = ReadInt32();

	float* floatAddr = reinterpret_cast<float *>(&container);

	return *floatAddr;
}

const std::string BufferView::ReadUTF16() {
	std::string result;
	char newChar;

	do {
		if (this->current >= this->end) {
			throw std::out_of_range(
				std::format("Buffer overflow at ReadUTF16 offset= {} final string length= {}", this->current - this->start, result.size())
			);
		}

		newChar = *this->current;

		this->current += 2;

		result += newChar;
	} while (newChar != '\0');

	result.pop_back();

	return result;
}

const std::string BufferView::ReadOffsetUTF16() {
	uint64_t offset = this->ReadInt64();
	auto currentOffset = this->GetOffset();

	this->SetOffset(offset);
	std::string result = this->ReadUTF16();
	this->SetOffset(currentOffset);

	return result;
}

const std::string BufferView::ReadOffsetUTF16(int offset) {
	auto currentOffset = this->GetOffset();

	this->SetOffset(offset);
	std::string result = this->ReadUTF16();
	this->SetOffset(currentOffset);

	return result;
}

void BufferView::AssertByte(byte expected, const std::string& message) {
	if (ReadByte() != expected) {
		throw std::runtime_error(message);
	}
}

void BufferView::AssertByte(byte expected) {
	if (ReadByte() != expected) {
		throw std::runtime_error(std::format("Assertion error at offset: {}", this->GetOffset()));
	}
}

void BufferView::AssertInt32(int expected, const std::string& message) {
	if (ReadInt32() != expected) {
		throw std::runtime_error(message);
	}
}

void BufferView::AssertInt32(int expected) {
	if (ReadInt32() != expected) {
		throw std::runtime_error(std::format("Assertion error at offset: {}", this->GetOffset()));
	}
}

void BufferView::AssertInt64(uint64_t expected, const std::string& message) {
	if (ReadInt64() != expected) {
		throw std::runtime_error(message);
	}
}

void BufferView::AssertInt64(uint64_t expected) {
	if (ReadInt64() != expected) {
		throw std::runtime_error(std::format("Assertion error at offset: {}", this->GetOffset()));
	}
}

void BufferView::AssertASCII(const std::string& expected, const std::string& message) {
	if (ReadASCII(expected.length()) != expected) {
		throw std::runtime_error(message);
	}
}

void BufferView::AssertASCII(const std::string& expected) {
	if (ReadASCII(expected.length()) != expected) {
		throw std::runtime_error(std::format("Assertion error at offset: {}", this->GetOffset()));
	}
}

void BufferView::AssertASCII(const std::string& expected, int explicitLength, const std::string& message) {
	auto s = ReadASCII(explicitLength);

	if (expected.length() < explicitLength) {
		std::string extendedExpected = expected;

		do {
			extendedExpected.push_back('\0');
		} while (extendedExpected.length() < explicitLength);

		if (s != extendedExpected) {
			throw std::runtime_error(std::format("{} expected: {}; got: {}", message, expected, s));
		}
	}
	else {
		if (s != expected) {
			throw std::runtime_error(std::format("{} expected: {}; got: {}", message, expected, s));
		}
	}
}

void BufferView::AssertASCII(const std::string& expected, int explicitLength) {
	auto s = ReadASCII(explicitLength);

	if (expected.length() < explicitLength) {
		std::string extendedExpected = expected;

		do {
			extendedExpected.push_back('\0');
		} while (extendedExpected.length() < explicitLength);

		if (s != extendedExpected) {
			throw std::runtime_error(std::format("Assertion error at offset: {} expected: {}; got: {}", GetOffset(), expected, s));
		}
	}
	else {
		if (s != expected) {
			throw std::runtime_error(std::format("Assertion error at offset: {} expected: {}; got: {}", GetOffset(), expected, s));
		}
	}
}

void BufferView::SetPos(byte* pos) {
	if (pos >= this->start && pos < this->end) {
		this->current = pos;
	}
	else {
		throw std::out_of_range(
			std::format("Position was out of range: {:x}", (intptr_t) pos)
		);
	}
}

void BufferView::Advance(int movement) {
	SetOffset(this->GetOffset() + movement);
}

void BufferView::WriteInt32(int value) {
	if (current + sizeof(int) > end) {
		throw std::out_of_range(
			std::format("Buffer overflow at Read offset= {} length= {}", this->current - this->start, sizeof(int))
		);
	}

	if (!bigEndian) {
		memcpy(current, (byte *) &value, sizeof(int));

		current += sizeof(int);
	}
	else {
		std::array<byte, sizeof(int)> resultArray = {
			(byte) value >> 24,
			(byte) value >> 16,
			(byte) value >> 8,
			(byte) value
		};
		
		Write(resultArray);
	}
}

void BufferView::WriteByte(byte value) {
	Write<1>({value});
}

void BufferView::WriteBool(bool value) {
	Write<1>({value});
}

void BufferView::WriteFloat(float value) {
	if (current + sizeof(int) > end) {
		throw std::out_of_range(
			std::format("Buffer overflow at Read offset= {} length= {}", this->current - this->start, sizeof(int))
		);
	}

	uint32_t byteValue = *reinterpret_cast<uint32_t *>(&value);

	if (!bigEndian) {
		memcpy(current, (byte *) &value, sizeof(int));

		current += sizeof(int);
	}
	else {
		std::array<byte, sizeof(int)> resultArray = {
			(byte) byteValue >> 24,
			(byte) byteValue >> 16,
			(byte) byteValue >> 8,
			(byte) byteValue
		};
		
		Write(resultArray);
	}
}

void BufferView::WriteInt64(uint64_t value) {
	if (current + sizeof(uint64_t) > end) {
		throw std::out_of_range(
			std::format("Buffer overflow at Read offset= {} length= {}", this->current - this->start, sizeof(uint64_t))
		);
	}

	if (!bigEndian) {
		memcpy(current, (byte *) &value, sizeof(uint64_t));

		current += sizeof(uint64_t);
	}
	else {
		std::array<byte, sizeof(uint64_t)> resultArray = {
			(byte) value >> 56,
			(byte) value >> 48,
			(byte) value >> 40,
			(byte) value >> 32,
			(byte) value >> 24,
			(byte) value >> 16,
			(byte) value >> 8,
			(byte) value
		};
		
		Write(resultArray);
	}
}

void BufferView::WriteASCII(const std::string& value, bool nullTerminate) {
	for (char c : value) {
		*this->current = c;

		this->Advance(1);
	}

	if (nullTerminate) {
		*this->current = '\0';

		this->Advance(1);
	}
}

void BufferView::WriteUTF16(const std::string& s) {
	for (char c : s) {
		*this->current = c;
		*(this->current + 1) = '\0';

		this->Advance(2);
	}

	*this->current = '\0';
	*(this->current + 1) = '\0';

	this->Advance(2);
}

void BufferView::SetOffset(size_t offset) {
	if (this->start + offset <= this->end) {
		this->current = const_cast<byte *>(this->start + offset);
	}
	else {
		throw std::out_of_range(
			std::format("Offset was out of range: {}", offset)
		);
	}
}

byte* BufferView::GetPos() {
	return this->current;
}

size_t BufferView::GetOffset() {
	return this->current - this->start;
}

bool BufferView::IsBigEndian() {
	return this->bigEndian;
}

void BufferView::SetBigEndian(bool setBigEndian) {
	this->bigEndian = setBigEndian;
}

void WriteUTF16ToStream(std::ostream& o, const std::string& s) {
	for (char c : s) {
		if (c) {
			o << c << '\0';
		}
	}

	o << '\0' << '\0';
}

void DumpToFile(const std::filesystem::path& p, const byte* start, const byte* end) {
	if (end < start) {
		return;
	}

	std::ofstream output(p, std::ios::binary);

	output.write((char *) start, end - start);

	output.close();
}
#pragma once

#include <array>
#include <string>
#include <iostream>
#include <cstring>
#include <format>
#include <stdint.h>
#include <filesystem>

typedef unsigned char byte;

class BufferView {
private:
	const byte* start;
	const byte* end;
	byte* current;

	bool bigEndian;
public:
	BufferView(const byte* start, const byte* end, bool bigEndian = true):
	start(start),
	end(end),
	current(const_cast<byte *>(start)),
	bigEndian(bigEndian) {}

	BufferView(const byte* start, size_t length, bool bigEndian = true):
	start(start),
	end(start + length),
	current(const_cast<byte *>(start)),
	bigEndian(bigEndian) {}

	template <size_t Size>
	const std::array<byte, Size> Read() {
		if (current + Size > end) {
			throw std::out_of_range(
				std::format("Buffer overflow at Read offset= {} length= {}", this->current - this->start, Size)
			);
		}

		std::array<byte, Size> result = {0};

		memcpy(result.data(), current, Size);

		current += Size;

		return result;
	}

	const std::string ReadASCII(int length);

	byte ReadByte();

	bool ReadBoolean();

	int ReadInt32();

	uint64_t ReadInt64();

	float ReadFloat();

	const std::string ReadUTF16();

	const std::string ReadOffsetUTF16();

	const std::string ReadOffsetUTF16(int offset);

	template<size_t Length>
	const std::array<bool, Length> ReadBoolArray() {
		std::array<bool, Length> result = {false};

		for (int i = 0; i < Length; i++) {
			result[i] = this->ReadByte() != 0;
		}

		return result;
	}

	template<size_t Length>
	const std::array<int, Length> ReadIntArray() {
		std::array<int, Length> result = {0};

		for (int i = 0; i < Length; i++) {
			result[i] = this->ReadInt32();
		}

		return result;
	}

	template<size_t Length>
	const std::array<float, Length> ReadFloatArray() {
		std::array<float, Length> result = {0};

		for (int i = 0; i < Length; i++) {
			result[i] = this->ReadFloat();
		}

		return result;
	}

	void AssertByte(byte expected, const std::string& message);

	void AssertByte(byte expected);

	void AssertInt32(int expected, const std::string& message);

	void AssertInt32(int expected);

	void AssertInt64(uint64_t expected, const std::string& message);

	void AssertInt64(uint64_t expected);

	void AssertASCII(const std::string& expected, const std::string& message);

	void AssertASCII(const std::string& expected);

	void AssertASCII(const std::string& expected, int explicitLength, const std::string& message);

	void AssertASCII(const std::string& expected, int explicitLength);

	void Advance(int movement);

	template<size_t Length>
	void Write(std::array<byte, Length> data) {
		if (current + Length > end) {
			throw std::out_of_range(
				std::format("Buffer overflow at Read offset= {} length= {}", this->current - this->start, Length)
			);
		}

		memcpy(current, data.data(), Length);

		current += Length;
	}

	void WriteInt32(int value);

	void WriteFloat(float value);

	void WriteInt64(uint64_t value);

	void WriteASCII(const std::string& value, bool nullTerminate = true);
	
	void WriteUTF16(const std::string& s);

	byte* GetPos();
	size_t GetOffset();

	template<typename T, size_t Count = 1>
	void Skip() {
		Advance(sizeof(T) * Count);
	}

	void SetPos(byte* pos);
	void SetOffset(size_t offset);

	bool IsBigEndian();
	void SetBigEndian(bool setBigEndian);
};


template <size_t Size>
std::ostream& operator<< (std::ostream& o, const std::array<byte, Size>& a) {
	for (int i = 0; i < Size; i++) {
		o.put(a[i]);
	}

	return o;
}

void WriteUTF16ToStream(std::ostream& o, const std::string& s);

template <size_t Size>
std::array<byte, Size> ToBytes(const std::string& s) {
	std::array<byte, Size> result = {0};

	memcpy(result.data(), s.c_str(), s.length());

	return result;
}

template<typename T>
requires requires (T x) { std::is_fundamental<T>::value == true; }
struct AsBytes {
	static std::array<byte, sizeof(T)> Bytes(T val, bool bigEndian = true) {
		std::array<byte, sizeof(T)> result = {0};

		if (bigEndian) {
			for (int i = 0; i < sizeof(T); i++) {
				result[i] = (byte) (val >> ((sizeof(T) - 1 - i) * 8));
			}
		}
		else {
			for (int i = 0; i < sizeof(T); i++) {
				result[i] = (byte) (val >> i * 8);
			}
		}

		return result;
	}
};

template<>
struct AsBytes<bool> {
	static std::array<byte, 1> Bytes(bool val, bool bigEndian = true) {
		return { val };
	}
};

template<typename T>
std::array<byte, sizeof(T)> ToBytes(T val, bool bigEndian = true) {
	return AsBytes<T>::Bytes(val, bigEndian);
}

void DumpToFile(const std::filesystem::path& p, const byte* start, const byte* end);
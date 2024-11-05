#pragma once

#include <array>
#include <string>
#include <iostream>
#include <cstring>

typedef unsigned char byte;

template <size_t Size>
const std::array<byte, Size> Extract(const byte* buffer, int& counter) {
	std::array<byte, Size> result = {0};

	memcpy(result.data(), buffer + counter, Size);

	counter += Size;

	return result;
}

template <size_t Length>
const std::string ExtractASCII(const byte* buffer, int& counter) {
	const std::array<byte, Length> strContainer = Extract<Length>(buffer, counter);

	std::string result;

	if (strContainer[Length - 1] == '\0') {
		result = std::string((char *) strContainer.data());
	}
	else {
		byte temp[Length + 1] = {0};

		memcpy(temp, strContainer.data(), Length);

		result = std::string((char *) temp);
	}

	return result;
}

int ExtractInt32(const byte* buffer, int& counter, bool bigEndian = true);

bool AssertInt(int got, int expected);

bool AssertInt(int got, int expected, const std::string& message);

bool AssertASCII(const std::string& got, const std::string& expected);

bool AssertASCII(const std::string& got, const std::string& expected, const std::string& message);

template <size_t Size>
std::ostream& operator<< (std::ostream& o, const std::array<byte, Size>& a) {
	for (int i = 0; i < Size; i++) {
		o.put(a[i]);
	}

	return o;
}

template <size_t Size>
std::array<byte, Size> ToBytes(const std::string& s) {
	std::array<byte, Size> result = {0};

	memcpy(result.data(), s.c_str(), s.length());

	return result;
}

std::array<byte, 4> ToBytes(int i, bool bigEndian = true);
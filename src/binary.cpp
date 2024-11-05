#include "binary.h"

#include "spdlog/spdlog.h"

int ExtractInt32(const byte* buffer, int& counter, bool bigEndian) {
	const std::array<byte, 4> intContainer = Extract<4>(buffer, counter);

	int result = 0;

	unsigned int b0 = intContainer[0];
	unsigned int b1 = intContainer[1];
	unsigned int b2 = intContainer[2];
	unsigned int b3 = intContainer[3];

	if (bigEndian) {
		result += (b0 << 24) | (b1 << 16) | (b2 << 8) | b3;
	}
	else {
		result += (b3 << 24) | (b2 << 16) | (b1 << 8) | b0;
	}

	return result;
}

bool AssertInt(int got, int expected) {
	return got == expected;
}

bool AssertInt(int got, int expected, const std::string& message) {
	bool result = AssertInt(got, expected);

	if (!result) {
		spdlog::error("Assertion failed - " + message + "; " + std::to_string(got) + " != " + std::to_string(expected));
	}

	return result;
}

bool AssertASCII(const std::string& got, const std::string& expected) {
	return got.compare(expected) == 0;
}

bool AssertASCII(const std::string& got, const std::string& expected, const std::string& message) {
	bool result = AssertASCII(got, expected);

	if (!result) {
		spdlog::error("Assertion failed - " + message + "; " + got + " != " + expected);
	}

	return result;
}

std::array<byte, 4> ToBytes(int i, bool bigEndian) {
	std::array<byte, 4> result = {0};

	if (bigEndian) {
		result[0] = (byte) (i >> 24);
		result[1] = (byte) (i >> 16);
		result[2] = (byte) (i >> 8);
		result[3] = (byte) i;
	}
	else {
		result[0] = (byte) i;
		result[1] = (byte) (i >> 8);
		result[2] = (byte) (i >> 16);
		result[3] = (byte) (i >> 24);
	}

	return result;
}
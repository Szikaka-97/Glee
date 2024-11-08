#pragma once

#include <array>

struct PropertyChangeValue {
	const bool enabled;
	float value;
};

struct PropertyChange {
	std::string target;
	std::array<PropertyChangeValue, 5> values;
};
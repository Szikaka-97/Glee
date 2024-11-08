#pragma once

#include <vector>
#include "pugixml.hpp"

#include "property_change.h"

class MaterialChange {
private:
	std::vector<PropertyChange> changes;
public:
	static MaterialChange* Parse(pugi::xml_node changeNode);

	const std::vector<PropertyChange>& GetChanges() const;

	void CopyChanges(MaterialChange* other);
};
#pragma once

#include <vector>
#include "pugixml.hpp"

#include "property_change.h"
#include "texture_change.h"

class MaterialChange {
private:
	std::vector<PropertyChange> propertyChanges;
	std::vector<TextureChange> textureChanges;
public:
	static MaterialChange* Parse(pugi::xml_node changeNode);

	const std::vector<PropertyChange>& GetPropertyChanges() const;
	const std::vector<TextureChange>& GetTextureChanges() const;

	void CopyChanges(MaterialChange* other);
};
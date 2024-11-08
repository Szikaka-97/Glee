#pragma once

#include <map>

#include "pugixml.hpp"

#include "material_change.h"

class MaterialMod {
private:
	std::map<std::string, MaterialChange*> changes;
public:
	MaterialMod();
	~MaterialMod();

	void AddMod(const pugi::xml_document& modXML);

	const std::map<std::string, MaterialChange*>& GetChanges() const;
};
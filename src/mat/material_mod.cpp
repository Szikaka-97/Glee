#include "material_mod.h"

#include <iostream>

MaterialMod::MaterialMod() {}

MaterialMod::~MaterialMod() {
	for (const auto& pair : this->changes) {
		if (pair.second) {
			delete pair.second;
		}
	}
}

void MaterialMod::AddMod(const pugi::xml_document& modXML) {
	auto root = modXML.child("material-mod");

	if (!root) {
		// ERROR - Bad XML structure

		return;
	}

	for (const auto& changeNode : root.children("material-change")) {
		std::string target = changeNode.attribute("target").as_string();

		if (target.empty()) {
			// ERROR - No target

			continue;
		}

		MaterialChange* matChange = MaterialChange::Parse(changeNode);

		if (!matChange) {
			// ERROR - Incorrect material-change

			continue;
		}

		if (this->changes.contains(target)) {
			this->changes[target]->CopyChanges(matChange);
		}
		else {
			this->changes[target] = matChange;
		}
	}
}

const std::map<std::string, MaterialChange*>& MaterialMod::GetChanges() const {
	return this->changes;
}
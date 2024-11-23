#include "material_change.h"

#include <regex>
#include <iostream>

#include "spdlog/spdlog.h"
#include "../utils.h"

PropertyChangeValue ReadNode(const pugi::xml_node& node) {
	const static std::regex floatRegex("^\\s*\\d+(\\.\\d+)?\\s*$");
	const static std::regex boolRegex("^\\s*(true)|(false)\\s*$", std::regex_constants::icase);

	auto nodeVal = std::string(node.text().as_string());

	if (std::regex_match(nodeVal, floatRegex)) {
		return { true, std::stof(nodeVal) };
	}
	else if (std::regex_match(nodeVal, boolRegex)) {
		return { true, (float) node.text().as_bool() };
	}

	return { false, 0 };
}

auto FindNodeMultipleNames(const std::initializer_list<std::string>& nodeNames) {
	return [nodeNames](const pugi::xml_node& child) {
		for (const std::string& name : nodeNames) {
			if(name == child.name()) {
				return true;
			}
		}

		return false;
	};
}

MaterialChange* MaterialChange::Parse(pugi::xml_node changeNode) {
	MaterialChange* result = new MaterialChange();

	for (const auto& propertyChangeNode : changeNode.children("property-change")) {
		std::string targetProperty = propertyChangeNode.attribute("target").as_string();

		if (targetProperty.empty()) {
			// ERROR - No property target

			continue;
		}

		const auto xPropNode = ReadNode(propertyChangeNode.find_child( FindNodeMultipleNames({"r", "x", "value"}) ));
		const auto yPropNode = ReadNode(propertyChangeNode.find_child( FindNodeMultipleNames({"g", "y"}) ));
		const auto zPropNode = ReadNode(propertyChangeNode.find_child( FindNodeMultipleNames({"b", "z"}) ));
		const auto wPropNode = ReadNode(propertyChangeNode.find_child( FindNodeMultipleNames({"a", "w"}) ));
		const auto qPropNode = ReadNode(propertyChangeNode.child("q"));
		
		result->propertyChanges.push_back({ targetProperty, {xPropNode, yPropNode, zPropNode, wPropNode, qPropNode} });
	}

	for (const auto& textureChangeNode : changeNode.children("texture-change")) {
		std::string targetProperty = textureChangeNode.attribute("target").as_string();

		if (targetProperty.empty()) {
			// ERROR - No texture target

			continue;
		}

		std::string replacementPath = StringUtils::Trim(textureChangeNode.child("path").text().as_string());

		result->textureChanges.push_back({ targetProperty, replacementPath });
	}

	return result;
}

const std::vector<PropertyChange>& MaterialChange::GetPropertyChanges() const {
	return this->propertyChanges;
}

const std::vector<TextureChange>& MaterialChange::GetTextureChanges() const {
	return this->textureChanges;
}

void MaterialChange::CopyChanges(MaterialChange* other) {
	for (const auto propChange : other->propertyChanges) {
		this->propertyChanges.push_back(propChange);
	}
	for (const auto texChange : other->textureChanges) {
		this->textureChanges.push_back(texChange);
	}
}
#pragma once

#include <string_view>

enum class DefinitionHashDataType;
int parse_class_skills_info(std::string_view buf, DefinitionHashDataType);

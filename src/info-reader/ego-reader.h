#pragma once

#include <string_view>

enum class DefinitionHashDataType;
int parse_egos_info(std::string_view buf, DefinitionHashDataType);

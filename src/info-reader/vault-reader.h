#pragma once

#include <string_view>

enum class DefinitionHashDataType;
int parse_vaults_info(std::string_view buf, DefinitionHashDataType);

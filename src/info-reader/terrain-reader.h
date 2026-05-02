#pragma once

#include "external-lib/include-json.h"

enum class DefinitionHashDataType;
int parse_terrains_json_info(nlohmann::json &element, DefinitionHashDataType);

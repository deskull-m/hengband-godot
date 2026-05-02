#pragma once

#include "external-lib/include-json.h"

enum class DefinitionHashDataType;
int parse_monraces_info(nlohmann::json &element, DefinitionHashDataType);

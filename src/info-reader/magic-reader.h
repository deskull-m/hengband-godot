#pragma once

#include "external-lib/include-json.h"

enum class DefinitionHashDataType;
int parse_class_magics_info(nlohmann::json &class_data, DefinitionHashDataType);

#pragma once

#include "external-lib/include-json.h"

enum class DefinitionHashDataType;
int parse_artifacts_info(nlohmann::json &element, DefinitionHashDataType);

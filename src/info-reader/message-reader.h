#pragma once

#include "external-lib/include-json.h"

enum class DefinitionHashDataType;
int parse_monster_messages_info(nlohmann::json &element, DefinitionHashDataType);

#pragma once

#include "external-lib/include-json.h"
#include "system/angband.h"
#include <string_view>

struct angband_header;
errr parse_dungeons_info(std::string_view buf, angband_header *head);

errr parse_dungeons_json_info(nlohmann::json &element, angband_header *head);

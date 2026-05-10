#pragma once

#include "system/angband.h"

#include <string_view>
#include <unordered_map>

enum class DungeonFeatureType;
enum class DungeonMode;
enum class PitKind;

extern const std::unordered_map<std::string_view, DungeonFeatureType> dungeon_flags;
extern const std::unordered_map<std::string_view, DungeonMode> dungeon_modes;
extern const std::unordered_map<std::string_view, PitKind> dungeon_pit_kinds;

#pragma once

#include "util/point-2d.h"

class ItemEntity;
class PlayerType;
ItemEntity *choose_warning_item(PlayerType *player_ptr);
bool process_warning(PlayerType *player_ptr, const Pos2D &pos);

/*!
 * @file godot-player-status.cpp
 * @brief プレイヤーステータスのGodot側へのブリッジ実装
 */

#include "godot-player-status.h"

#include "player-info/class-info.h"
#include "player-info/race-info.h"
#include "system/floor/floor-info.h"
#include "system/gamevalue.h"
#include "system/player-type-definition.h"

#include <mutex>

namespace {

std::mutex g_status_mutex;
GodotPlayerStatusSnapshot g_snapshot;
uint64_t g_version_counter{ 0 };

} // namespace

void player_status_push(const PlayerType *player_ptr)
{
    if (!player_ptr) {
        return;
    }

    GodotPlayerStatusSnapshot snap;
    snap.is_valid = true;
    snap.version = ++g_version_counter;

    snap.name = player_ptr->name;

    if (rp_ptr) {
        snap.race_name = rp_ptr->title.string();
    }
    if (cp_ptr) {
        snap.class_name = cp_ptr->title.string();
    }

    snap.level = player_ptr->lev;
    snap.dun_level = player_ptr->current_floor_ptr
                         ? player_ptr->current_floor_ptr->dun_level
                         : 0;

    snap.chp = player_ptr->chp;
    snap.mhp = player_ptr->mhp;
    snap.csp = static_cast<int>(player_ptr->csp);
    snap.msp = player_ptr->msp;

    snap.gold = static_cast<long>(player_ptr->au);
    snap.exp = static_cast<long>(player_ptr->exp);
    snap.max_exp = static_cast<long>(player_ptr->max_exp);

    snap.speed = player_ptr->pspeed - STANDARD_SPEED;
    snap.display_ac = player_ptr->dis_ac + player_ptr->dis_to_a;

    for (int i = 0; i < 6; ++i) {
        snap.stat_use[i] = player_ptr->stat_use[i];
        snap.stat_top[i] = player_ptr->stat_top[i];
    }

    std::lock_guard<std::mutex> lock(g_status_mutex);
    g_snapshot = std::move(snap);
}

GodotPlayerStatusSnapshot player_status_get_snapshot()
{
    std::lock_guard<std::mutex> lock(g_status_mutex);
    return g_snapshot;
}

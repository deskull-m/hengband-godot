/*!
 * @file godot-init.cpp
 * @brief Godot 版ゲーム初期化実装
 */

#include "godot-init.h"

#include "main/angband-initializer.h"
#include "core/game-play.h"
#include "term/z-term.h"
#include "term/gameterm.h"
#include "io/files-util.h"
#include "system/system-variables.h"
#include "system/player-type-definition.h"

#include <filesystem>

namespace hengband_godot {

void init_godot_game_paths(const std::filesystem::path &lib_path)
{
    init_file_paths(lib_path);

    // システム識別子 (プリファレンスファイル選択に使用される)
    ANGBAND_SYS = "godot";
    ANGBAND_KEYBOARD = "JAPAN";
}

void run_game_thread(const std::filesystem::path &lib_path)
{
    init_godot_game_paths(lib_path);

    term_activate(term_screen);

    init_angband(p_ptr, false);

    // セーブファイルが指定されていなければ新規ゲーム
    const bool new_game = savefile.empty();
    play_game(p_ptr, new_game, false);

    quit(nullptr);
}

} // namespace hengband_godot

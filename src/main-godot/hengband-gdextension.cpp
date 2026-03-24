/*!
 * @file hengband-gdextension.cpp
 * @brief Hengband GDExtension 実装 + エントリポイント
 */

#include "hengband-gdextension.h"
#include "godot-terminal.h"
#include "godot-term-hooks.h"

#include "term/z-term.h"
#include "term/gameterm.h"

#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>

#include <string>

using namespace godot;
using namespace hengband_godot;

// ---------------------------------------------------------------------------
// HengbandGame 実装
// ---------------------------------------------------------------------------

void HengbandGame::_ready()
{
    // シーン内の GodotTerminal ノードを TerminalContainer から探す
    Node *container = get_node_or_null(NodePath("TerminalContainer"));
    if (!container) {
        return;
    }

    for (int i = 0; i < HENGBAND_TERM_COUNT; ++i) {
        const std::string node_name = "Terminal" + std::to_string(i);
        auto *term = Object::cast_to<GodotTerminal>(
            container->get_node_or_null(NodePath(node_name.c_str())));
        if (term) {
            setup_terminal(i, term, (i == 0) ? 80 : 80, (i == 0) ? 24 : 24);
            if (font_.is_valid()) {
                term->set_terminal_font(font_, font_size_);
            }
        }
    }

    // game_term をメインターミナルに設定
    if (term_data_[0].terminal) {
        term_activate(&term_data_[0].t);
    }

    // TODO: Phase 7 でゲームスレッドを起動する
}

void HengbandGame::_process(double delta)
{
    (void)delta;
    // TODO: Phase 4 でキュー処理を実装する
}

void HengbandGame::_notification(int p_what)
{
    if (p_what == NOTIFICATION_WM_CLOSE_REQUEST) {
        // TODO: ゲームスレッドへの終了シグナル
    }
}

void HengbandGame::start_game()
{
    // TODO: Phase 7 で play_game() 呼び出しを実装する
}

void HengbandGame::set_game_font(const Ref<Font> &font, int size)
{
    font_ = font;
    font_size_ = size;
}

void HengbandGame::setup_terminal(int idx, GodotTerminal *term, int cols, int rows)
{
    auto &td = term_data_[idx];
    td.terminal = term;
    td.cols = cols;
    td.rows = rows;

    term->set_grid_size(cols, rows);

    term_type *t = &td.t;
    term_init(t, cols, rows, 256);
    t->soft_cursor = true;
    t->attr_blank = 1; // TERM_WHITE
    t->char_blank = ' ';
    t->init_hook = term_init_godot;
    t->nuke_hook = term_nuke_godot;
    t->text_hook = term_text_godot;
    t->wipe_hook = term_wipe_godot;
    t->curs_hook = term_curs_godot;
    t->xtra_hook = term_xtra_godot;
    t->data = static_cast<vptr>(&td);

    // angband_term に登録（ゲームロジック側から参照される）
    if (idx < HENGBAND_TERM_COUNT) {
        angband_term[idx] = t;
    }
}

void HengbandGame::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("start_game"), &HengbandGame::start_game);
    ClassDB::bind_method(D_METHOD("set_game_font", "font", "size"),
        &HengbandGame::set_game_font);
}

// ---------------------------------------------------------------------------
// GDExtension エントリポイント
// ---------------------------------------------------------------------------

void initialize_hengband_module(ModuleInitializationLevel p_level)
{
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
        return;
    }
    ClassDB::register_class<GodotTerminal>();
    ClassDB::register_class<HengbandGame>();
}

void uninitialize_hengband_module(ModuleInitializationLevel p_level)
{
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
        return;
    }
}

extern "C" {

GDExtensionBool GDE_EXPORT hengband_gdextension_init(
    GDExtensionInterfaceGetProcAddress p_get_proc_address,
    const GDExtensionClassLibraryPtr p_library,
    GDExtensionInitialization *r_initialization)
{
    godot::GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);
    init_obj.register_initializer(initialize_hengband_module);
    init_obj.register_terminator(uninitialize_hengband_module);
    init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);
    return init_obj.init();
}

} // extern "C"

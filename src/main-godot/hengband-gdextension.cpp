/*!
 * @file hengband-gdextension.cpp
 * @brief Hengband GDExtension 実装 + エントリポイント
 */

#include "hengband-gdextension.h"
#include "godot-terminal.h"
#include "godot-tile-layer.h"
#include "godot-input-handler.h"
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
    // TerminalContainer 以下から Terminal0〜7 と TileLayer0〜7 を探して設定
    Node *container = get_node_or_null(NodePath("TerminalContainer"));
    if (!container) {
        return;
    }

    for (int i = 0; i < HENGBAND_TERM_COUNT; ++i) {
        const std::string term_name = "Terminal" + std::to_string(i);
        const std::string tile_name = "TileLayer" + std::to_string(i);

        auto *term = Object::cast_to<GodotTerminal>(
            container->get_node_or_null(NodePath(term_name.c_str())));
        auto *tiles = Object::cast_to<GodotTileLayer>(
            container->get_node_or_null(NodePath(tile_name.c_str())));

        if (term) {
            setup_terminal(i, term, tiles, 80, 24);
            if (font_.is_valid()) {
                term->set_terminal_font(font_, font_size_);
            }
        }
    }

    // game_term をメインターミナルに設定
    if (term_data_[0].terminal) {
        term_activate(&term_data_[0].t);
    }

    // InputHandler を取得して TERM_XTRA_EVENT に登録
    auto *input_handler = Object::cast_to<GodotInputHandler>(
        get_node_or_null(NodePath("InputHandler")));
    if (input_handler) {
        set_input_handler(input_handler);
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

bool HengbandGame::load_tileset(const String &tileset_path,
    const String &mask_path,
    int cell_w, int cell_h)
{
    // メインターミナルの TileLayer にタイルセットを読み込む
    // tile_w / tile_h はフォントサイズに合わせる（cell_w と同じ値でよい）
    auto *tiles = term_data_[0].tile_layer;
    if (!tiles) {
        return false;
    }
    const bool ok = tiles->load_tileset(
        tileset_path.utf8().get_data(),
        mask_path.utf8().get_data(),
        cell_w, cell_h, cell_w, cell_h);

    if (ok) {
        // pict_hook を有効化
        term_data_[0].t.higher_pict = true;
    }
    return ok;
}

void HengbandGame::setup_terminal(int idx, GodotTerminal *term,
    GodotTileLayer *tiles, int cols, int rows)
{
    auto &td = term_data_[idx];
    td.terminal = term;
    td.tile_layer = tiles;
    td.cols = cols;
    td.rows = rows;

    term->set_grid_size(cols, rows);
    if (tiles) {
        tiles->set_grid_size(cols, rows);
    }

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
    t->pict_hook = term_pict_godot;
    t->xtra_hook = term_xtra_godot;
    t->data = static_cast<vptr>(&td);

    if (idx < HENGBAND_TERM_COUNT) {
        angband_term[idx] = t;
    }
}

void HengbandGame::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("start_game"), &HengbandGame::start_game);
    ClassDB::bind_method(D_METHOD("set_game_font", "font", "size"),
        &HengbandGame::set_game_font);
    ClassDB::bind_method(
        D_METHOD("load_tileset", "tileset_path", "mask_path", "cell_w", "cell_h"),
        &HengbandGame::load_tileset);
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
    ClassDB::register_class<GodotTileLayer>();
    ClassDB::register_class<GodotInputHandler>();
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

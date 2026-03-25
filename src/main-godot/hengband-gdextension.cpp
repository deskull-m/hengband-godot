/*!
 * @file hengband-gdextension.cpp
 * @brief Hengband GDExtension 実装 + エントリポイント
 */

#include "hengband-gdextension.h"
#include "godot-audio-manager.h"
#include "godot-init.h"
#include "godot-terminal.h"
#include "godot-tile-layer.h"
#include "godot-input-handler.h"
#include "godot-term-hooks.h"

#include "term/z-term.h"
#include "term/gameterm.h"

#include <godot_cpp/classes/callable.hpp>
#include <godot_cpp/classes/config_file.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>

#include <filesystem>
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

        // サブウィンドウのルートノード(Terminal側)を記録
        sub_window_roots_[i] = term;

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
    input_handler_ = Object::cast_to<GodotInputHandler>(
        get_node_or_null(NodePath("InputHandler")));
    if (input_handler_) {
        set_input_handler(input_handler_);
    }

    // term_data 配列を TERM_XTRA_REACT / resize_hook に登録
    set_term_data_array(term_data_.data(), HENGBAND_TERM_COUNT);
}

void HengbandGame::_process(double delta)
{
    (void)delta;
}

void HengbandGame::_notification(int p_what)
{
    if (p_what == NOTIFICATION_WM_CLOSE_REQUEST) {
        // InputHandler のブロック待機を解除してスレッドを終了
        if (input_handler_) {
            input_handler_->request_stop();
        }
        if (game_thread_.is_valid() && game_thread_->is_started()) {
            game_thread_->wait_to_finish();
        }
    }
}

void HengbandGame::start_game(const String &lib_path)
{
    if (game_thread_.is_valid() && game_thread_->is_started()) {
        return; // 既に起動中
    }

    // lib_path が空の場合は実行ファイルの隣の lib/ を使う
    String resolved_lib;
    if (lib_path.is_empty()) {
        const String exe = OS::get_singleton()->get_executable_path();
        resolved_lib = exe.get_base_dir().path_join("lib");
    } else {
        resolved_lib = lib_path;
    }

    game_thread_.instantiate();
    game_thread_->start(
        Callable(this, "_game_thread_func"),
        Variant(resolved_lib),
        Thread::PRIORITY_NORMAL);
}

/// ゲームスレッド本体 (Godot Thread から呼ばれる)
void HengbandGame::_game_thread_func(String lib_path)
{
    const std::filesystem::path lib_fs(lib_path.utf8().get_data());
    run_game_thread(lib_fs);
}
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
    t->resize_hook = term_resize_hook_godot;
    t->data = static_cast<vptr>(&td);

    if (idx < HENGBAND_TERM_COUNT) {
        angband_term[idx] = t;
    }
}

void HengbandGame::set_sub_window_visible(int idx, bool visible)
{
    if (idx < 0 || idx >= HENGBAND_TERM_COUNT) {
        return;
    }
    auto *root = sub_window_roots_[idx];
    if (root) {
        root->set_visible(visible);
    }
    // TileLayer も同期
    auto *tiles = term_data_[idx].tile_layer;
    if (tiles) {
        tiles->set_visible(visible);
    }
}

void HengbandGame::set_sub_window_size(int idx, int cols, int rows)
{
    if (idx < 0 || idx >= HENGBAND_TERM_COUNT) {
        return;
    }
    auto &td = term_data_[idx];
    if (cols <= 0 || rows <= 0) {
        return;
    }
    td.cols = cols;
    td.rows = rows;
    if (td.terminal) {
        td.terminal->set_grid_size(cols, rows);
    }
    if (td.tile_layer) {
        td.tile_layer->set_grid_size(cols, rows);
    }
    // term_resize で z-term 側にも反映
    term_type *saved = game_term;
    term_activate(&td.t);
    term_resize(cols, rows);
    if (saved) {
        term_activate(saved);
    }
}

void HengbandGame::save_window_layout(const String &path)
{
    Ref<ConfigFile> cfg;
    cfg.instantiate();

    // SubViewport の親ノードから位置を取得して保存
    Node *container = get_node_or_null(NodePath("GameViewport/SubViewport/TerminalContainer"));

    for (int i = 0; i < HENGBAND_TERM_COUNT; ++i) {
        const std::string section = "window" + std::to_string(i);
        const String sec(section.c_str());

        auto *root = sub_window_roots_[i];
        cfg->set_value(sec, "visible", root ? root->is_visible() : (i == 0));
        cfg->set_value(sec, "cols", term_data_[i].cols);
        cfg->set_value(sec, "rows", term_data_[i].rows);

        if (root) {
            cfg->set_value(sec, "pos_x", static_cast<int>(root->get_position().x));
            cfg->set_value(sec, "pos_y", static_cast<int>(root->get_position().y));
        }
    }
    cfg->save(path);
}

void HengbandGame::load_window_layout(const String &path)
{
    Ref<ConfigFile> cfg;
    cfg.instantiate();
    if (cfg->load(path) != Error::OK) {
        return;
    }

    for (int i = 0; i < HENGBAND_TERM_COUNT; ++i) {
        const std::string section = "window" + std::to_string(i);
        const String sec(section.c_str());

        const bool visible = cfg->get_value(sec, "visible", i == 0);
        const int cols = cfg->get_value(sec, "cols", 80);
        const int rows = cfg->get_value(sec, "rows", 24);
        const int pos_x = cfg->get_value(sec, "pos_x", 0);
        const int pos_y = cfg->get_value(sec, "pos_y", 0);

        set_sub_window_visible(i, visible);
        if (cols > 0 && rows > 0) {
            set_sub_window_size(i, cols, rows);
        }
        auto *root = sub_window_roots_[i];
        if (root) {
            root->set_position(Vector2(static_cast<float>(pos_x),
                static_cast<float>(pos_y)));
        }
    }
}

void HengbandGame::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("start_game", "lib_path"), &HengbandGame::start_game);
    ClassDB::bind_method(D_METHOD("set_game_font", "font", "size"),
        &HengbandGame::set_game_font);
    ClassDB::bind_method(
        D_METHOD("load_tileset", "tileset_path", "mask_path", "cell_w", "cell_h"),
        &HengbandGame::load_tileset);
    ClassDB::bind_method(
        D_METHOD("set_sub_window_visible", "idx", "visible"),
        &HengbandGame::set_sub_window_visible);
    ClassDB::bind_method(
        D_METHOD("set_sub_window_size", "idx", "cols", "rows"),
        &HengbandGame::set_sub_window_size);
    ClassDB::bind_method(
        D_METHOD("save_window_layout", "path"),
        &HengbandGame::save_window_layout);
    ClassDB::bind_method(
        D_METHOD("load_window_layout", "path"),
        &HengbandGame::load_window_layout);
    ClassDB::bind_method(
        D_METHOD("_game_thread_func", "exe_path"),
        &HengbandGame::_game_thread_func);
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
    ClassDB::register_class<GodotAudioManager>();
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

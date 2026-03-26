/*!
 * @file godot-term-hooks.cpp
 * @brief term_type フック関数の Godot 実装
 */

#include "godot-term-hooks.h"
#include "godot-audio-manager.h"
#include "godot-input-handler.h"
#include "godot-terminal.h"
#include "godot-tile-layer.h"

#include "term/z-term.h"
#include "locale/japanese.h"

#include <thread>
#include <chrono>

using namespace hengband_godot;

/// シーン内の GodotInputHandler への参照（HengbandGame が設定する）
static GodotInputHandler *s_input_handler = nullptr;
/// term_data_godot 配列への参照（TERM_XTRA_REACT / resize_hook 用）
static term_data_godot *s_term_data = nullptr;
static int s_term_count = 0;

void set_input_handler(GodotInputHandler *handler)
{
    s_input_handler = handler;
}

void set_term_data_array(term_data_godot *arr, int count)
{
    s_term_data = arr;
    s_term_count = count;
}

// ---------------------------------------------------------------------------
// resize_hook: ゲームがターミナルサイズを変更した時のコールバック
// ---------------------------------------------------------------------------
void term_resize_hook_godot()
{
    if (!game_term || !game_term->data) {
        return;
    }
    auto *td = reinterpret_cast<term_data_godot *>(game_term->data);
    const int new_cols = static_cast<int>(game_term->wid);
    const int new_rows = static_cast<int>(game_term->hgt);
    if (td->cols == new_cols && td->rows == new_rows) {
        return;
    }
    td->cols = new_cols;
    td->rows = new_rows;
    if (td->terminal) {
        td->terminal->set_grid_size(new_cols, new_rows);
    }
    if (td->tile_layer) {
        td->tile_layer->set_grid_size(new_cols, new_rows);
    }
}

// ---------------------------------------------------------------------------
// text_hook: テキスト描画
// ---------------------------------------------------------------------------
errr term_text_godot(int x, int y, int n, TERM_COLOR a, concptr s)
{
    auto *term = get_terminal(game_term);
    if (!term) {
        return 1;
    }
#ifdef JP
    // z-term から渡される s は SJIS(CP932) エンコード、n はバイト数 = セル数。
    // Godot のフォント描画は UTF-8 を要求するため変換する。
    // n バイト分だけを変換対象とし、null 終端に依存しない。
    const auto utf8 = sys_to_utf8(std::string_view(s, static_cast<size_t>(n)));
    const char *str = utf8 ? utf8->c_str() : s;
    term->draw_text(x, y, n, static_cast<uint8_t>(a), str);
#else
    term->draw_text(x, y, n, static_cast<uint8_t>(a), s);
#endif
    return 0;
}

// ---------------------------------------------------------------------------
// wipe_hook: セル消去
// ---------------------------------------------------------------------------
errr term_wipe_godot(int x, int y, int n)
{
    auto *term = get_terminal(game_term);
    if (!term) {
        return 1;
    }
    term->wipe_cells(x, y, n);
    return 0;
}

// ---------------------------------------------------------------------------
// curs_hook: カーソル描画
// ---------------------------------------------------------------------------
errr term_curs_godot(int x, int y)
{
    auto *term = get_terminal(game_term);
    if (!term) {
        return 1;
    }
    term->set_cursor(x, y);
    return 0;
}

// ---------------------------------------------------------------------------
// xtra_hook: 拡張アクション
// ---------------------------------------------------------------------------
errr term_xtra_godot(int n, int v)
{
    switch (n) {
    case TERM_XTRA_CLEAR: {
        auto *term = get_terminal(game_term);
        if (term) {
            term->clear_all();
        }
        auto *tiles = get_tile_layer(game_term);
        if (tiles) {
            tiles->clear_all();
        }
        return 0;
    }
    case TERM_XTRA_FRESH:
    case TERM_XTRA_FROSH: {
        // 全ターミナルの再描画をメインスレッドにキュー
        auto *term = get_terminal(game_term);
        if (term) {
            term->call_deferred("queue_redraw");
        }
        auto *tiles = get_tile_layer(game_term);
        if (tiles) {
            tiles->call_deferred("queue_redraw");
        }
        return 0;
    }
    case TERM_XTRA_SHAPE:
        // カーソル表示/非表示 (v=0 で非表示)
        if (v == 0) {
            auto *term = get_terminal(game_term);
            if (term) {
                term->hide_cursor();
            }
        }
        return 0;
    case TERM_XTRA_DELAY:
        // v = ミリ秒
        // Phase 4: シングルスレッドのため sleep で実装
        // Phase 7: ゲームスレッド内での sleep に置き換える
        if (v > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(v));
        }
        return 0;
    case TERM_XTRA_EVENT:
        // v=true: キーが来るまで待機、v=false: ノンブロッキング
        if (s_input_handler) {
            if (v) {
                s_input_handler->wait_for_key(); // Phase 4: 即時リターン
            } else {
                s_input_handler->poll_events();
            }
        }
        return 0;
    case TERM_XTRA_FLUSH:
    case TERM_XTRA_BORED:
    case TERM_XTRA_ALIVE:
    case TERM_XTRA_LEVEL:
        return 0;
    case TERM_XTRA_REACT:
        // カラーテーブル更新 + 全ターミナルのサイズ同期
        if (s_term_data) {
            for (int i = 0; i < s_term_count; ++i) {
                auto &td = s_term_data[i];
                const int w = static_cast<int>(td.t.wid);
                const int h = static_cast<int>(td.t.hgt);
                if (w > 0 && h > 0 && (td.cols != w || td.rows != h)) {
                    td.cols = w;
                    td.rows = h;
                    if (td.terminal) {
                        td.terminal->set_grid_size(w, h);
                    }
                    if (td.tile_layer) {
                        td.tile_layer->set_grid_size(w, h);
                    }
                }
            }
        }
        return 0;
    case TERM_XTRA_SOUND:
        hengband_godot::audio_play_sound(v);
        return 0;
    case TERM_XTRA_MUSIC_BASIC:
    case TERM_XTRA_MUSIC_DUNGEON:
    case TERM_XTRA_MUSIC_QUEST:
    case TERM_XTRA_MUSIC_TOWN:
    case TERM_XTRA_MUSIC_MONSTER:
        hengband_godot::audio_play_music(n, v);
        return 0;
    case TERM_XTRA_MUSIC_MUTE:
        hengband_godot::audio_stop_music();
        return 0;
    default:
        return 1; // 未サポート
    }
}

// ---------------------------------------------------------------------------
// pict_hook: タイル描画
// ---------------------------------------------------------------------------
errr term_pict_godot(TERM_LEN x, TERM_LEN y, int n,
    const TERM_COLOR *ap, concptr cp,
    const TERM_COLOR *tap, concptr tcp)
{
    auto *tiles = get_tile_layer(game_term);
    if (!tiles || !tiles->is_tileset_loaded()) {
        // タイルセット未ロード時はテキスト消去で代替
        return term_wipe_godot(x, y, n);
    }
    tiles->draw_tiles(x, y, n,
        reinterpret_cast<const uint8_t *>(ap), cp,
        reinterpret_cast<const uint8_t *>(tap), tcp);
    return 0;
}

// ---------------------------------------------------------------------------
// init_hook / nuke_hook
// ---------------------------------------------------------------------------
void term_init_godot(term_type *t)
{
    // GodotTerminal の設定は HengbandGame::_ready() 側で行う
    (void)t;
}

void term_nuke_godot(term_type *t)
{
    // GodotTerminal はシーンのライフサイクルで管理されるため何もしない
    (void)t;
}

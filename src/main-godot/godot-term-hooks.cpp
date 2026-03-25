/*!
 * @file godot-term-hooks.cpp
 * @brief term_type フック関数の Godot 実装
 */

#include "godot-term-hooks.h"
#include "godot-terminal.h"
#include "godot-tile-layer.h"

#include "term/z-term.h"

using namespace hengband_godot;

// ---------------------------------------------------------------------------
// text_hook: テキスト描画
// ---------------------------------------------------------------------------
errr term_text_godot(int x, int y, int n, TERM_COLOR a, concptr s)
{
    auto *term = get_terminal(game_term);
    if (!term) {
        return 1;
    }
    term->draw_text(x, y, n, static_cast<uint8_t>(a), s);
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
// Phase 2 では TERM_XTRA_CLEAR のみ実装。他は後続 Phase で実装。
// ---------------------------------------------------------------------------
errr term_xtra_godot(int n, int v)
{
    (void)v;
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
    case TERM_XTRA_FROSH:
        // Godot の queue_redraw() で対応済みなので何もしない
        return 0;
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
        // TODO: Phase 4 でスレッドセーフな待機を実装
        return 0;
    case TERM_XTRA_EVENT:
    case TERM_XTRA_FLUSH:
    case TERM_XTRA_BORED:
    case TERM_XTRA_REACT:
    case TERM_XTRA_ALIVE:
    case TERM_XTRA_LEVEL:
        // TODO: Phase 4・5 で実装
        return 0;
    case TERM_XTRA_SOUND:
    case TERM_XTRA_MUSIC_BASIC:
    case TERM_XTRA_MUSIC_DUNGEON:
    case TERM_XTRA_MUSIC_QUEST:
    case TERM_XTRA_MUSIC_TOWN:
    case TERM_XTRA_MUSIC_MONSTER:
    case TERM_XTRA_MUSIC_MUTE:
        // TODO: Phase 5 で AudioStreamPlayer を使って実装
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

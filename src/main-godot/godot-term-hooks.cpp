/*!
 * @file godot-term-hooks.cpp
 * @brief term_type フック関数の Godot 実装
 */

#include "godot-term-hooks.h"
#include "godot-audio-manager.h"
#include "godot-input-handler.h"
#include "godot-map3d.h"
#include "godot-player-status.h"
#include "godot-terminal.h"
#include "godot-tile-layer.h"

#include "core/visuals-reseter.h"
#include "game-option/runtime-arguments.h"
#include "game-option/special-options.h"
#include "locale/japanese.h"
#include "system/enums/terrain/terrain-characteristics.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/item/item-entity.h"
#include "system/monrace/monrace-definition.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "system/system-variables.h"
#include "system/terrain/terrain-definition.h"
#include "target/target-checker.h"
#include "term/gameterm.h"
#include "term/z-term.h"
#include "view/display-symbol.h"
#include "window/main-window-util.h"

#include <atomic>
#include <chrono>
#include <thread>

using namespace hengband_godot;

/// シーン内の GodotInputHandler への参照（HengbandGame が設定する）
static GodotInputHandler *s_input_handler = nullptr;
/// term_data_godot 配列への参照（TERM_XTRA_REACT / resize_hook 用）
static term_data_godot *s_term_data = nullptr;
static int s_term_count = 0;

/// Godot スレッドからタイルモードが変更されたことをゲームスレッドに伝えるフラグ
/// apply_tile_mode() がセット → TERM_XTRA_REACT がクリアして reset_visuals() を呼ぶ
static std::atomic<bool> s_graphics_mode_changed{ false };

/// ビッグタイルの ON/OFF が変化したことをゲームスレッドに伝えるフラグ
/// apply_bigtile_mode() がセット → TERM_XTRA_REACT がクリアして resize_map() を呼ぶ
static std::atomic<bool> s_bigtile_mode_changed{ false };

void set_input_handler(GodotInputHandler *handler)
{
    s_input_handler = handler;
}

void set_term_data_array(term_data_godot *arr, int count)
{
    s_term_data = arr;
    s_term_count = count;
}

void notify_screen_save_depth(int depth)
{
    // メインターミナル (term 0) のみ 3D オーバーレイ対象なのでそこにだけ通知する。
    if (!s_term_data || s_term_count <= 0) {
        return;
    }

    auto *terminal = s_term_data[0].terminal;
    if (terminal) {
        terminal->set_screen_save_mode(depth);
    }
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
    // map3d はターミナルサイズに依存しない (フロアスナップショット駆動)
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
#else
    const char *str = s;
#endif
    term->draw_text(x, y, n, static_cast<uint8_t>(a), str);
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
        // 3D マップは TERM_XTRA_FRESH 時にフロアスナップショットで再構築されるため
        // ここで明示的にクリアする必要はない (set_active(false) で破棄される)
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
        // メインターミナル (term 0) の更新時にプレイヤーステータスを同期する
        if (game_term == term_screen) {
            player_status_push(p_ptr);
        }
        // 3D マップ: フロア全体のスナップショットを生成して送る。
        // ターミナルパネルの可視範囲に縛られない、ダンジョン全体表示用。
        if (auto *m3d = get_map3d(game_term)) {
            if (p_ptr && p_ptr->current_floor_ptr) {
                const auto &floor = *p_ptr->current_floor_ptr;
                const int w = static_cast<int>(floor.width);
                const int h = static_cast<int>(floor.height);
                // floor.width/height と grid_array の実サイズはロード中やフロア生成前に
                // 食い違うことがある。その状態で grid_array[dy][dx] に触れると領域外参照で
                // クラッシュするため、全行が w 列以上あることを確認できた時だけ走査する。
                bool grid_ready = (w > 0 && h > 0 && static_cast<int>(floor.grid_array.size()) >= h);
                if (grid_ready) {
                    for (int dy = 0; dy < h; ++dy) {
                        if (static_cast<int>(floor.grid_array[dy].size()) < w) {
                            grid_ready = false;
                            break;
                        }
                    }
                }
                // 上限を設けて暴走防止 (Hengband の最大は概ね 198×66 = 13068)
                if (grid_ready && (static_cast<long long>(w) * h) < 100000LL) {
                    std::vector<uint8_t> kinds(static_cast<size_t>(w) * h, 0);
                    // タイルモード時は地形の表示シンボル (attr, char) も同梱して
                    // 3D 側で同じタイル画像から albedo を切り出せるようにする。
                    // (attr, char) はそれぞれ 0x7F でマスクすると 2D pict_hook と一致する。
                    const bool tile_mode = (use_graphics != 0);
                    std::vector<uint8_t> tile_syms;
                    if (tile_mode) {
                        tile_syms.assign(static_cast<size_t>(w) * h * 2, 0);
                    }
                    for (int dy = 0; dy < h; ++dy) {
                        for (int dx = 0; dx < w; ++dx) {
                            const auto &g = floor.grid_array[dy][dx];
                            // プレイヤーがまだ知らないセルは描画しない
                            if (!g.is_mark() && !g.is_view()) {
                                continue;
                            }
                            const auto &terrain = g.get_terrain();
                            const auto &tf = terrain.flags;
                            // 特徴的な地形を先に判定し、最後に壁/岩石/床へ振り分ける。
                            // (扉・階段はFLOORフラグも持つため壁/床より先に判定する)
                            uint8_t kind = M3D_FLOOR; // 既定は床
                            if (tf.has(TerrainCharacteristics::DOOR)) {
                                // 通行可能 (MOVE) な扉 = 開いた/壊れた扉、それ以外 = 閉じた扉
                                kind = tf.has(TerrainCharacteristics::MOVE) ? M3D_DOOR_OPEN : M3D_DOOR_CLOSED;
                            } else if (tf.has(TerrainCharacteristics::UP_STAIRS)) {
                                kind = M3D_STAIR_UP;
                            } else if (tf.has(TerrainCharacteristics::DOWN_STAIRS)) {
                                kind = M3D_STAIR_DOWN;
                            } else if (tf.has(TerrainCharacteristics::TREE)) {
                                kind = M3D_TREE;
                            } else if (tf.has(TerrainCharacteristics::LAVA)) {
                                kind = M3D_LAVA;
                            } else if (tf.has(TerrainCharacteristics::WATER)) {
                                kind = M3D_WATER;
                            } else if (tf.has(TerrainCharacteristics::MOUNTAIN)) {
                                kind = M3D_MOUNTAIN;
                            } else if (tf.has(TerrainCharacteristics::WALL)) {
                                // STONE 持ちの壁 = 鉱脈 (マグマ/石英)、それ以外は通常の壁
                                kind = tf.has(TerrainCharacteristics::STONE) ? M3D_VEIN : M3D_WALL;
                            } else if (tf.has(TerrainCharacteristics::STONE)) {
                                // 壁ではないが STONE を持つ = 岩石 (rubble)
                                kind = M3D_RUBBLE;
                            }
                            kinds[dy * w + dx] = kind;
                            if (tile_mode) {
                                // F_LIT_STANDARD の symbol_configs を参照。
                                // 暗所/明所で多少色が違うが 3D マップは全体俯瞰なので
                                // 標準ライティングで統一する。
                                const auto sym_it = terrain.symbol_configs.find(F_LIT_STANDARD);
                                if (sym_it != terrain.symbol_configs.end()) {
                                    const auto &sym = sym_it->second;
                                    tile_syms[(dy * w + dx) * 2 + 0] = static_cast<uint8_t>(sym.color & 0x7F);
                                    tile_syms[(dy * w + dx) * 2 + 1] = static_cast<uint8_t>(sym.character & 0x7F);
                                }
                            }
                        }
                    }

                    // 視認中のモンスターを収集する
                    // (m_list[0] は予約済みのプレイヤースロットなので 1 から)
                    std::vector<Map3DMonster> monsters;
                    const int mmax = static_cast<int>(floor.m_max);
                    monsters.reserve(static_cast<size_t>(mmax));
                    for (int m_idx = 1; m_idx < mmax; ++m_idx) {
                        const auto &mon = floor.m_list[m_idx];
                        if (!mon.is_valid()) {
                            continue;
                        }
                        if (!mon.ml) {
                            continue; // 視認外 (含むテレパシーで未補足)
                        }
                        const auto &monrace = mon.get_monrace();
                        const auto &sym = monrace.symbol_config;
                        if (!sym.has_character()) {
                            continue;
                        }
                        Map3DMonster ent;
                        ent.m_idx = m_idx;
                        ent.x = static_cast<int>(mon.fx);
                        ent.y = static_cast<int>(mon.fy);
                        ent.ch = sym.character;
                        ent.color = sym.color;
                        monsters.push_back(ent);
                    }

                    // 床上アイテムを収集する (モンスターが所持しているものは除く)
                    std::vector<Map3DItem> items;
                    const int omax = static_cast<int>(floor.o_list.size());
                    items.reserve(static_cast<size_t>(omax));
                    for (int o_idx = 1; o_idx < omax; ++o_idx) {
                        const auto &item_ptr = floor.o_list[o_idx];
                        if (!item_ptr) {
                            continue;
                        }
                        const auto &item = *item_ptr;
                        if (!item.is_valid()) {
                            continue;
                        }
                        if (item.is_held_by_monster()) {
                            continue;
                        }
                        const int dx = static_cast<int>(item.ix);
                        const int dy = static_cast<int>(item.iy);
                        if (dx <= 0 || dy <= 0 || dx >= w || dy >= h) {
                            continue;
                        }
                        // 視認できないセルのアイテムは表示しない
                        const auto &g = floor.grid_array[dy][dx];
                        if (!g.is_mark() && !g.is_view()) {
                            continue;
                        }
                        const auto sym = item.get_symbol();
                        if (!sym.has_character()) {
                            continue;
                        }
                        Map3DItem ent;
                        ent.o_idx = o_idx;
                        ent.x = dx;
                        ent.y = dy;
                        ent.ch = sym.character;
                        ent.color = sym.color;
                        items.push_back(ent);
                    }

                    m3d->set_floor_snapshot(w, h, kinds.data(),
                        static_cast<int>(p_ptr->x), static_cast<int>(p_ptr->y),
                        monsters.empty() ? nullptr : monsters.data(),
                        static_cast<int>(monsters.size()),
                        items.empty() ? nullptr : items.data(),
                        static_cast<int>(items.size()),
                        tile_syms.empty() ? nullptr : tile_syms.data());
                }
            }
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
        // Windows 版 term_xtra_win_react() 相当:
        //   1. タイルモードが変化していれば prf ファイルを再ロード (reset_visuals)
        //      ※ このケースは必ずゲームスレッドから呼ばれるので安全
        //   2. 全ターミナルのサイズ同期
        if (s_graphics_mode_changed.exchange(false) && p_ptr) {
            reset_visuals(p_ptr);
        }
        // ビッグタイル変更時: パネル境界のみ再計算し、スクロール幅を補正する。
        // resize_map() は内部で term_redraw() を呼ぶため、
        // do_cmd_redraw 内から TERM_XTRA_REACT に来た場合に二重クリアが起きる。
        // そのため verify_panel() だけ呼び、描画は do_cmd_redraw 本体に任せる。
        if (s_bigtile_mode_changed.exchange(false) && p_ptr) {
            panel_row_max = 0;
            panel_col_max = 0;
            panel_row_min = p_ptr->current_floor_ptr->height;
            panel_col_min = p_ptr->current_floor_ptr->width;
            verify_panel(p_ptr);
        }
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

    // タイルを描画する位置のテキスト層セルを空白にする。
    // GodotTerminal は空白セルを描画スキップするため、タイル層が透けて見える。
    auto *term = get_terminal(game_term);
    if (term) {
        term->wipe_cells(x, y, n);
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

// ---------------------------------------------------------------------------
// タイルモード切り替えユーティリティ
// ---------------------------------------------------------------------------

void apply_bigtile_mode(bool enabled)
{
    arg_bigtile = enabled;
    use_bigtile = enabled;
    // ゲームスレッドに panel_col_max の再計算を依頼する
    s_bigtile_mode_changed.store(true);
    s_graphics_mode_changed.store(true);
}

void apply_tile_mode(bool enabled, const char *graf_name)
{
    // フラグ類の設定は Godot メインスレッドから呼ばれても安全な単純代入のみ行う。
    // reset_visuals() の呼び出しはゲームスレッドで TERM_XTRA_REACT が処理する。

    use_graphics = enabled;
    // graf_name が一時バッファを指している場合に備えて、
    // ANGBAND_GRAF には必ず静的な文字列リテラルを代入する。
    if (!enabled) {
        ANGBAND_GRAF = "ascii";
    } else if (strcmp(graf_name, "new") == 0) {
        ANGBAND_GRAF = "new";
    } else if (strcmp(graf_name, "ne2") == 0) {
        ANGBAND_GRAF = "ne2";
    } else {
        ANGBAND_GRAF = "old"; // "old" またはデフォルト
    }

    // term_screen の higher_pict フラグを更新する
    if (term_screen) {
        term_screen->higher_pict = enabled;
    }

    // ゲームスレッドへの通知: 次の TERM_XTRA_REACT で reset_visuals() を呼ばせる
    s_graphics_mode_changed.store(true);
}

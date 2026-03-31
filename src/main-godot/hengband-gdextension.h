#pragma once
/*!
 * @file hengband-gdextension.h
 * @brief Hengband GDExtension メインヘッダ
 *
 * Godot Engine の GDExtension として Hengband を統合するための
 * エントリポイントおよびコアクラス宣言。
 */

#include "godot-audio-manager.h"
#include "godot-terminal.h"
#include "godot-tile-layer.h"
#include "godot-input-handler.h"
#include "godot-term-hooks.h"

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/classes/thread.hpp>
#include <godot_cpp/classes/font.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <array>
#include <atomic>

/// ターミナル数（メイン + サブウィンドウ最大8）
static constexpr int HENGBAND_TERM_COUNT = 8;

namespace hengband_godot {

/*!
 * @brief Hengband ゲームのGodotノード
 *
 * Godot シーンに配置し、ゲームロジックのライフサイクルを管理する。
 * ゲームループは別スレッドで実行される（FR-07参照）。
 */
class HengbandGame : public godot::Node {
    GDCLASS(HengbandGame, godot::Node)

public:
    HengbandGame() = default;
    ~HengbandGame() override = default;

    void _ready() override;
    void _process(double delta) override;
    void _notification(int p_what);

    /// ゲームを開始する (Godot Thread を起動して play_game() を呼ぶ)
    void start_game();

    /// ゲームスレッド本体 (Callable として Thread に渡す)
    void _game_thread_func(godot::String exe_path);

    /// フォントを設定する（GDScript から呼び出す）
    void set_game_font(const godot::Ref<godot::Font> &font, int size);

    /// サブウィンドウ(idx=1〜7)の表示/非表示を切り替える
    void set_sub_window_visible(int idx, bool visible);

    /// サブウィンドウのターミナルサイズを変更する
    void set_sub_window_size(int idx, int cols, int rows);

    /// ウィンドウレイアウト(表示状態・位置)をConfigFileに保存する
    void save_window_layout(const godot::String &path);

    /// ウィンドウレイアウトをConfigFileから復元する
    void load_window_layout(const godot::String &path);

protected:
    static void _bind_methods();

private:
    /// 各ターミナルの term_data_godot
    std::array<term_data_godot, HENGBAND_TERM_COUNT> term_data_{};

    /// サブウィンドウ(idx=1〜7)の Node2D ルートノード
    std::array<godot::Node2D *, HENGBAND_TERM_COUNT> sub_window_roots_{};

    /// ゲームスレッド (Phase 7)
    godot::Ref<godot::Thread> game_thread_;
    /// 入力ハンドラへのキャッシュ (停止通知用)
    GodotInputHandler *input_handler_{ nullptr };

    /// 使用するフォント
    godot::Ref<godot::Font> font_;
    int font_size_{ 14 };

    /// タイルセットを読み込む（GDScript から呼び出す）
    bool load_tileset(const godot::String &tileset_path,
        const godot::String &mask_path,
        int cell_w, int cell_h);

    /// ターミナルを初期化して term_type フックを設定する
    void setup_terminal(int idx, GodotTerminal *term,
        GodotTileLayer *tiles, int cols, int rows);
};

} // namespace hengband_godot

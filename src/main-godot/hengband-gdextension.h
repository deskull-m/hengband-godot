#pragma once
/*!
 * @file hengband-gdextension.h
 * @brief Hengband GDExtension メインヘッダ
 *
 * Godot Engine の GDExtension として Hengband を統合するための
 * エントリポイントおよびコアクラス宣言。
 */

#include "godot-terminal.h"
#include "godot-tile-layer.h"
#include "godot-term-hooks.h"

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/font.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <array>

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

    /// ゲームを開始する（別スレッドから呼び出す）
    void start_game();

    /// フォントを設定する（GDScript から呼び出す）
    void set_game_font(const godot::Ref<godot::Font> &font, int size);

protected:
    static void _bind_methods();

private:
    /// 各ターミナルの term_data_godot
    std::array<term_data_godot, HENGBAND_TERM_COUNT> term_data_{};

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

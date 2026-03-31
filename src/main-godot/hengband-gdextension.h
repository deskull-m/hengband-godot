#pragma once
/*!
 * @file hengband-gdextension.h
 * @brief Hengband GDExtension メインヘッダ
 *
 * Godot Engine の GDExtension として Hengband を統合するための
 * エントリポイントおよびコアクラス宣言。
 */

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/core/class_db.hpp>

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

protected:
    static void _bind_methods();
};

} // namespace hengband_godot

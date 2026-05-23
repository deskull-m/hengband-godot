#pragma once
/*!
 * @file godot-map3d.h
 * @brief Godot 3D マップ描画ノード (Phase 1: テキスト入力ベースのスパイク)
 *
 * Hengband メイン端末の「マップ矩形」(COL_MAP, ROW_MAP 以降) に描かれる
 * テキスト/タイル情報を Minecraft 風のボクセル 3D 表示に変換する。
 *
 * Phase 1 設計:
 *  - 入力: term_text_godot から (x, y, char, color) を受け取りグリッドに格納
 *  - 出力: 文字種に応じて Mesh ノードを生成 / 配置
 *      '#'        → 高さ 2 の壁ボクセル
 *      '.', ','   → 高さ 0.05 の床ブロック
 *      '+', '\''  → 高さ 2 の扉ボクセル (色違い)
 *      '<', '>'   → 高さ 0.3 の階段マーカー
 *      '@'        → プレイヤー: 高さ 1.5 の赤いキューブ
 *      他の文字   → 高さ 1.0 のモンスター/アイテムキューブ (色は TERM_COLOR)
 *      ' ', 0     → 何も描画しない (闇)
 *
 *  - 1 セル = 1 MeshInstance3D (Phase 2 で MultiMesh に置き換え予定)
 *  - 描画更新は queue_rebuild() フラグで _process から呼び出す
 *
 * 座標系:
 *  - グリッド (gx, gy) → 3D (x, 0, z) = ((gx - COL_MAP), 0, (gy - ROW_MAP))
 *  - Y 軸が上方向（Godot 標準）
 */

#include <atomic>
#include <cstdint>
#include <godot_cpp/classes/node3d.hpp>
#include <mutex>
#include <string>
#include <vector>

namespace hengband_godot {

/// マップ 3D グリッドの 1 セルデータ
struct Map3DCell {
    char32_t ch = U' '; ///< 表示文字 (Unicode コードポイント)
    uint8_t color = 0; ///< TERM_COLOR インデックス
};

/*!
 * @brief Hengband マップ 3D 描画ノード
 *
 * Godot シーンの SubViewport(3D 有効) 配下に配置する。
 * 同じ SubViewport 内に Camera3D / DirectionalLight3D を置くこと。
 */
class GodotMap3D : public godot::Node3D {
    GDCLASS(GodotMap3D, godot::Node3D)

public:
    GodotMap3D();
    ~GodotMap3D() override = default;

    void _ready() override;
    void _process(double delta) override;

    /// グリッドサイズを設定する (テキスト端末のサイズに合わせる)
    void set_grid_size(int cols, int rows);

    /// マップ矩形の原点を設定する (デフォルト: COL_MAP=12, ROW_MAP=1)
    void set_map_origin(int col, int row);

    /// 端末セル (x, y) に文字を書き込む (text_hook から呼ばれる)
    /// @param x, y     端末上のセル座標
    /// @param n        セル数 (str を n セル分使う)
    /// @param color    TERM_COLOR インデックス
    /// @param str      UTF-8 文字列
    void update_text(int x, int y, int n, uint8_t color, const char *str);

    /// 端末セル (x, y) から n セルを空白に戻す (wipe_hook から呼ばれる)
    void wipe_cells(int x, int y, int n);

    /// 全セルをクリアする
    void clear_all();

    /// プレイヤー位置を取得する (Godot の `_process` 等で Camera 追従に使う)
    /// 戻り値は 3D ワールド座標 (cell 中心)
    godot::Vector3 get_player_world_position() const;

protected:
    static void _bind_methods();

private:
    int cols_{ 80 };
    int rows_{ 24 };
    int origin_col_{ 12 }; ///< COL_MAP デフォルト
    int origin_row_{ 1 }; ///< ROW_MAP デフォルト

    std::vector<Map3DCell> grid_; ///< (cols × rows) のセルデータ
    mutable std::mutex grid_mutex_; ///< ゲーム ↔ Godot メインスレッド競合防止

    /// 更新フラグ (true なら次の _process で rebuild_meshes() を呼ぶ)
    std::atomic<bool> dirty_{ false };

    /// 現在のプレイヤー位置 (グリッド座標、見つからない場合は -1)
    int player_x_{ -1 };
    int player_y_{ -1 };

    int cell_idx(int x, int y) const
    {
        return y * cols_ + x;
    }

    void resize_grid();

    /// 現在のグリッドからメッシュノードを全再生成する
    /// (Phase 1 簡易実装: 既存子ノードを削除して 1 セル 1 MeshInstance3D を追加)
    void rebuild_meshes();
};

} // namespace hengband_godot

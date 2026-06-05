#pragma once
/*!
 * @file godot-terminal.h
 * @brief Godot ターミナル描画ノード
 *
 * Hengband の term_type フック（text_hook / wipe_hook / curs_hook）を
 * Godot の Node2D ベース描画 API で実装する。
 *
 * 設計:
 *  - 内部に文字グリッド (cols × rows) のバッファを保持する
 *  - text_hook / wipe_hook でバッファを更新して queue_redraw() を呼ぶ
 *  - _draw() でフォントを使って全セルをバッチ描画する
 *  - 複数端末（最大8）はそれぞれ独立した GodotTerminal ノードとして扱う
 */

#include <cstdint>
#include <godot_cpp/classes/font.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/string.hpp>
#include <mutex>
#include <string>
#include <vector>

namespace hengband_godot {

/// 1セルのデータ
struct CellData {
    char32_t ch = U' '; ///< Unicode コードポイント
    uint8_t color = 0; ///< TERM_COLOR インデックス
};

/*!
 * @brief Hengband ターミナル描画ノード
 *
 * Godot シーンに配置して使用する。
 * term_type フックから draw_text / wipe_cells / draw_cursor を呼び出す。
 */
class GodotTerminal : public godot::Node2D {
    GDCLASS(GodotTerminal, godot::Node2D)

public:
    GodotTerminal();
    ~GodotTerminal() override = default;

    // --- Godot ライフサイクル ---
    void _ready() override;
    void _draw() override;

    // --- ターミナル設定 ---

    /// グリッドサイズを設定する
    void set_grid_size(int cols, int rows);

    /// セルサイズ（ピクセル）を設定する
    void set_cell_size(int w, int h);

    /// フォントを設定する
    void set_terminal_font(const godot::Ref<godot::Font> &font, int font_size);

    /// 透過モードを切り替える (3D マップ表示との重ね描画用)
    /// true: セル背景の黒塗りを停止し、ノード全体を半透明 (modulate.a = alpha) で描画する
    void set_transparent_mode(bool enabled, float alpha = 0.5f);

    /// screen_save モードを設定する (cコマンド等で一時的に画面を退避する時に有効化)
    /// 有効時は透過モードに関わらず:
    ///   - self_modulate.a = 1.0 で文字を不透明化
    ///   - 各文字に黒縁を追加して 3D オーバーレイ上でも視認性を確保
    /// 値は screen_save() の入れ子深度 (0 で無効、1 以上で有効)
    void set_screen_save_mode(int depth);

    // --- term_type フックから呼ばれる描画メソッド ---

    /// テキストを描画する（text_hook 相当）
    /// @param x   列インデックス
    /// @param y   行インデックス
    /// @param n   文字数
    /// @param color  TERM_COLOR インデックス
    /// @param str    描画する文字列（UTF-8）
    void draw_text(int x, int y, int n, uint8_t color, const char *str);

    /// セルを消去する（wipe_hook 相当）
    /// @param x  列インデックス
    /// @param y  行インデックス
    /// @param n  消去するセル数
    void wipe_cells(int x, int y, int n);

    /// カーソルを描画する（curs_hook 相当）
    /// @param x  列インデックス
    /// @param y  行インデックス
    void set_cursor(int x, int y);

    /// カーソルを非表示にする
    void hide_cursor();

    /// ターミナル全体を消去する
    void clear_all();

    // --- GDScript 公開プロパティ ---
    int get_cols() const
    {
        return cols_;
    }
    int get_rows() const
    {
        return rows_;
    }
    int get_cell_width() const
    {
        return cell_w_;
    }
    int get_cell_height() const
    {
        return cell_h_;
    }

protected:
    static void _bind_methods();

private:
    int cols_{ 80 };
    int rows_{ 24 };
    int cell_w_{ 8 };
    int cell_h_{ 16 };
    int font_size_{ 14 };

    bool cursor_visible_{ false };
    int cursor_x_{ 0 };
    int cursor_y_{ 0 };

    /// 透過モード (3D オーバーレイとの併用時に有効化)
    /// セル背景の黒塗りをスキップし、self_modulate.a を transparent_alpha_ にする
    bool transparent_mode_{ false };
    float transparent_alpha_{ 0.5f };

    /// screen_save() の入れ子深度。> 0 の間は不透明 + 黒縁モードを強制する
    int screen_save_depth_{ 0 };
    /// 文字の黒縁サイズ (ピクセル) screen_save モード時のみ使用
    static constexpr int kScreenSaveOutlineSize = 1;

    godot::Ref<godot::Font> font_;
    godot::Ref<godot::ImageTexture> wall_texture_; ///< wall.bmp 由来の 8x8 パターンテクスチャ
    std::vector<CellData> grid_; ///< グリッドバッファ (cols × rows)
    mutable std::mutex grid_mutex_; ///< ゲームスレッド ↔ Godotメインスレッド競合防止

    /// グリッド座標 → バッファインデックス
    int cell_idx(int x, int y) const
    {
        return y * cols_ + x;
    }

    /// バッファを (cols × rows) でリサイズし空白で初期化する
    void resize_grid();

    /// wall.bmp パターンから ImageTexture を生成する
    void create_wall_texture();

    void draw_cell(int x, int y, const CellData &cell);
    void draw_cursor_rect(int x, int y);

    /// 透過/screen_save の状態に応じた self_modulate を反映する (要メインスレッド)
    void apply_effective_modulate();
    /// call_deferred で呼ぶためのメインスレッド側ディスパッチ
    void _apply_modulate_deferred();
};

} // namespace hengband_godot

/*!
 * @file godot-terminal.cpp
 * @brief Godot ターミナル描画ノード実装
 */

#include "godot-terminal.h"
#include "term-color-map.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/classes/font.hpp>

#include <string>
#include <cstring>

using namespace godot;
using namespace hengband_godot;

// --- ユーティリティ: UTF-8 → UTF-32 変換 ---

/// UTF-8 文字列の最初の n 文字を UTF-32 コードポイント列に変換する
static std::vector<char32_t> utf8_to_codepoints(const char *str, int max_chars)
{
    std::vector<char32_t> result;
    result.reserve(max_chars);
    const unsigned char *s = reinterpret_cast<const unsigned char *>(str);
    while (*s && static_cast<int>(result.size()) < max_chars) {
        char32_t cp = 0;
        if (*s < 0x80) {
            cp = *s++;
        } else if ((*s & 0xE0) == 0xC0) {
            cp = (*s++ & 0x1F) << 6;
            cp |= (*s++ & 0x3F);
        } else if ((*s & 0xF0) == 0xE0) {
            cp = (*s++ & 0x0F) << 12;
            cp |= (*s++ & 0x3F) << 6;
            cp |= (*s++ & 0x3F);
        } else if ((*s & 0xF8) == 0xF0) {
            cp = (*s++ & 0x07) << 18;
            cp |= (*s++ & 0x3F) << 12;
            cp |= (*s++ & 0x3F) << 6;
            cp |= (*s++ & 0x3F);
        } else {
            ++s; // 不正なシーケンスをスキップ
            cp = U'?';
        }
        result.push_back(cp);
    }
    return result;
}

// --- GodotTerminal 実装 ---

GodotTerminal::GodotTerminal()
{
    resize_grid();
}

void GodotTerminal::_ready()
{
    resize_grid();
}

void GodotTerminal::_draw()
{
    if (!font_.is_valid()) {
        return;
    }

    // グリッドのスナップショットを取得 (ゲームスレッドとの競合防止)
    std::vector<CellData> snapshot;
    int snap_cols, snap_rows;
    {
        std::lock_guard<std::mutex> lock(grid_mutex_);
        snapshot = grid_;
        snap_cols = cols_;
        snap_rows = rows_;
    }

    // 背景を黒で塗りつぶす
    const Rect2 bg_rect(0, 0,
        static_cast<float>(snap_cols * cell_w_),
        static_cast<float>(snap_rows * cell_h_));
    draw_rect(bg_rect, Color(0, 0, 0));

    // 全セルを描画
    for (int y = 0; y < snap_rows; ++y) {
        for (int x = 0; x < snap_cols; ++x) {
            draw_cell(x, y, snapshot[y * snap_cols + x]);
        }
    }

    // カーソル描画
    if (cursor_visible_) {
        draw_cursor_rect(cursor_x_, cursor_y_);
    }
}

void GodotTerminal::draw_cell(int x, int y, const CellData &cell)
{
    if (cell.ch == U' ' || cell.ch == 0) {
        return;
    }

    const Color fg = term_color_to_godot(cell.color);

    // Godot の Font::draw_char() でセルに1文字描画
    // 基準点: セルの左下 (baseline)
    const Vector2 pos(
        static_cast<float>(x * cell_w_),
        static_cast<float>(y * cell_h_ + font_size_));

    // 背景色でセルを塗る（文字の後ろ）
    // TERM_DARK(黒)以外の文字はセル背景を黒にする
    draw_rect(Rect2(
        static_cast<float>(x * cell_w_),
        static_cast<float>(y * cell_h_),
        static_cast<float>(cell_w_),
        static_cast<float>(cell_h_)), Color(0, 0, 0));

    font_->draw_char(*this, pos, static_cast<int64_t>(cell.ch), font_size_, fg);
}

void GodotTerminal::draw_cursor_rect(int x, int y)
{
    // 黄色の矩形カーソルを描画
    const Rect2 rc(
        static_cast<float>(x * cell_w_),
        static_cast<float>(y * cell_h_),
        static_cast<float>(cell_w_),
        static_cast<float>(cell_h_));
    draw_rect(rc, Color(1.0f, 1.0f, 0.0f), false, 1.5f);
}

void GodotTerminal::set_grid_size(int cols, int rows)
{
    cols_ = cols;
    rows_ = rows;
    resize_grid();
    queue_redraw();
}

void GodotTerminal::set_cell_size(int w, int h)
{
    cell_w_ = w;
    cell_h_ = h;
    queue_redraw();
}

void GodotTerminal::set_terminal_font(const Ref<Font> &font, int font_size)
{
    font_ = font;
    font_size_ = font_size;
    queue_redraw();
}

void GodotTerminal::draw_text(int x, int y, int n, uint8_t color, const char *str)
{
    if (y < 0 || y >= rows_ || x < 0) {
        return;
    }

    const auto codepoints = utf8_to_codepoints(str, n);
    {
        std::lock_guard<std::mutex> lock(grid_mutex_);
        int col = x;
        for (const char32_t cp : codepoints) {
            if (col >= cols_) {
                break;
            }
            auto &cell = grid_[cell_idx(col, y)];
            cell.ch = cp;
            cell.color = color;
            ++col;
        }
    }
    queue_redraw();
}

void GodotTerminal::wipe_cells(int x, int y, int n)
{
    if (y < 0 || y >= rows_) {
        return;
    }
    {
        std::lock_guard<std::mutex> lock(grid_mutex_);
        for (int i = 0; i < n && (x + i) < cols_; ++i) {
            auto &cell = grid_[cell_idx(x + i, y)];
            cell.ch = U' ';
            cell.color = 0; // TERM_DARK
        }
    }
    queue_redraw();
}

void GodotTerminal::set_cursor(int x, int y)
{
    cursor_x_ = x;
    cursor_y_ = y;
    cursor_visible_ = true;
    queue_redraw();
}

void GodotTerminal::hide_cursor()
{
    cursor_visible_ = false;
    queue_redraw();
}

void GodotTerminal::clear_all()
{
    {
        std::lock_guard<std::mutex> lock(grid_mutex_);
        for (auto &cell : grid_) {
            cell.ch = U' ';
            cell.color = 0;
        }
    }
    cursor_visible_ = false;
    queue_redraw();
}

void GodotTerminal::resize_grid()
{
    std::lock_guard<std::mutex> lock(grid_mutex_);
    grid_.assign(static_cast<size_t>(cols_ * rows_), CellData{});
}

void GodotTerminal::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_grid_size", "cols", "rows"),
        &GodotTerminal::set_grid_size);
    ClassDB::bind_method(D_METHOD("set_cell_size", "width", "height"),
        &GodotTerminal::set_cell_size);
    ClassDB::bind_method(D_METHOD("set_terminal_font", "font", "size"),
        &GodotTerminal::set_terminal_font);
    ClassDB::bind_method(D_METHOD("draw_text", "x", "y", "n", "color", "str"),
        &GodotTerminal::draw_text);
    ClassDB::bind_method(D_METHOD("wipe_cells", "x", "y", "n"),
        &GodotTerminal::wipe_cells);
    ClassDB::bind_method(D_METHOD("set_cursor", "x", "y"),
        &GodotTerminal::set_cursor);
    ClassDB::bind_method(D_METHOD("hide_cursor"),
        &GodotTerminal::hide_cursor);
    ClassDB::bind_method(D_METHOD("clear_all"),
        &GodotTerminal::clear_all);
    ClassDB::bind_method(D_METHOD("get_cols"), &GodotTerminal::get_cols);
    ClassDB::bind_method(D_METHOD("get_rows"), &GodotTerminal::get_rows);
    ClassDB::bind_method(D_METHOD("get_cell_width"), &GodotTerminal::get_cell_width);
    ClassDB::bind_method(D_METHOD("get_cell_height"), &GodotTerminal::get_cell_height);
}

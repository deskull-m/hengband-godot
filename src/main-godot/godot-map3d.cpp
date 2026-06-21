/*!
 * @file godot-map3d.cpp
 * @brief Godot 3D マップ描画ノード実装 (Phase 1: テキスト入力ベース)
 */

#include "godot-map3d.h"
#include "term-color-map.h"

#include <godot_cpp/classes/box_mesh.hpp>
#include <godot_cpp/classes/material.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/standard_material3d.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <algorithm>

using namespace godot;
using namespace hengband_godot;

// ---------------------------------------------------------------------------
// セル種別 (rebuild_meshes でマテリアルキャッシュキーに使う)
// ---------------------------------------------------------------------------
enum MeshKind : uint16_t {
    KIND_NONE = 0,
    KIND_FLOOR = 1, ///< '.', ',' — 床ブロック (低)
    KIND_WALL = 2, ///< '#' — 高い壁
    KIND_DOOR_CLOSED = 3, ///< '+' — 閉扉
    KIND_DOOR_OPEN = 4, ///< '\'' — 開扉
    KIND_STAIR_UP = 5, ///< '<' — 上り階段
    KIND_STAIR_DOWN = 6, ///< '>' — 下り階段
    KIND_PLAYER = 7, ///< '@' — プレイヤー
    KIND_MONSTER = 8, ///< その他文字 — モンスター/アイテム
};

namespace {

struct MeshSpec {
    float size_x{ 0.9f };
    float size_y{ 0.05f };
    float size_z{ 0.9f };
    float y_offset{ 0.025f }; ///< AABB 中心の Y 位置
};

/// 文字を MeshKind に分類する
MeshKind classify_char(char32_t ch)
{
    switch (ch) {
    case U'.':
    case U',':
        return KIND_FLOOR;
    case U'#':
    case 0x7F: // wall (ASCII DEL: Hengband 内部での壁文字)
        return KIND_WALL;
    case U'+':
        return KIND_DOOR_CLOSED;
    case U'\'':
        return KIND_DOOR_OPEN;
    case U'<':
        return KIND_STAIR_UP;
    case U'>':
        return KIND_STAIR_DOWN;
    case U'@':
        return KIND_PLAYER;
    case U' ':
    case 0:
        return KIND_NONE;
    default:
        return KIND_MONSTER;
    }
}

MeshSpec spec_for_kind(MeshKind kind)
{
    MeshSpec s;
    switch (kind) {
    case KIND_FLOOR:
        s = { 0.98f, 0.05f, 0.98f, 0.025f };
        break;
    case KIND_WALL:
        s = { 1.0f, 2.0f, 1.0f, 1.0f };
        break;
    case KIND_DOOR_CLOSED:
        s = { 0.9f, 2.0f, 0.9f, 1.0f };
        break;
    case KIND_DOOR_OPEN:
        s = { 0.2f, 2.0f, 0.9f, 1.0f };
        break;
    case KIND_STAIR_UP:
    case KIND_STAIR_DOWN:
        s = { 0.8f, 0.3f, 0.8f, 0.15f };
        break;
    case KIND_PLAYER:
        s = { 0.6f, 1.6f, 0.6f, 0.8f };
        break;
    case KIND_MONSTER:
        s = { 0.7f, 1.0f, 0.7f, 0.5f };
        break;
    default:
        break;
    }
    return s;
}

/// MeshKind から基本色を生成する (TERM_COLOR と組み合わせる前のベース色)
Color base_color_for_kind(MeshKind kind, uint8_t term_color)
{
    // 一部の種別はゲームのカラーよりも目で見やすい固定色を優先する
    switch (kind) {
    case KIND_WALL:
        return Color(0.55f, 0.45f, 0.35f); // 茶土色
    case KIND_FLOOR:
        return Color(0.30f, 0.30f, 0.32f); // 暗い灰色
    case KIND_DOOR_CLOSED:
        return Color(0.60f, 0.30f, 0.10f); // ブラウン
    case KIND_DOOR_OPEN:
        return Color(0.40f, 0.25f, 0.10f);
    case KIND_STAIR_UP:
        return Color(0.50f, 0.80f, 1.00f); // 水色
    case KIND_STAIR_DOWN:
        return Color(1.00f, 0.70f, 0.20f); // オレンジ
    case KIND_PLAYER:
        return Color(1.00f, 0.20f, 0.20f); // 赤
    case KIND_MONSTER:
        // モンスター/アイテムは TERM_COLOR をそのまま使う
        return term_color_to_godot(term_color);
    default:
        return Color(1, 1, 1);
    }
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// GodotMap3D 実装
// ---------------------------------------------------------------------------

GodotMap3D::GodotMap3D()
{
    resize_grid();
}

void GodotMap3D::_ready()
{
    resize_grid();
}

void GodotMap3D::_process(double delta)
{
    // 非アクティブ (3D オーバーレイ非表示) のときはメッシュ再生成をスキップする。
    // 毎フレーム数百個の Mesh を作り直して固まるのを防ぐため。
    // dirty フラグはクリアせず、再アクティブ化時に最新状態へ追従できるようにする。
    if (!active_.load()) {
        return;
    }
    if (rebuild_cooldown_ > 0.0) {
        rebuild_cooldown_ -= delta;
        return;
    }
    if (dirty_.exchange(false)) {
        rebuild_meshes();
        rebuild_cooldown_ = REBUILD_INTERVAL;
    }
}

void GodotMap3D::set_active(bool active)
{
    const bool prev = active_.exchange(active);
    if (active && !prev) {
        // 非アクティブ→アクティブ遷移時は強制再構築
        // 直前の非アクティブ化前に残ったクールダウンで再構築が遅延しないよう 0 に戻す
        rebuild_cooldown_ = 0.0;
        dirty_.store(true);
    } else if (!active && prev) {
        // アクティブ→非アクティブ遷移時は既存メッシュを破棄してメモリ解放
        const int child_count = get_child_count();
        for (int i = child_count - 1; i >= 0; --i) {
            Node *child = get_child(i);
            if (child) {
                remove_child(child);
                child->queue_free();
            }
        }
    }
}

void GodotMap3D::set_grid_size(int cols, int rows)
{
    if (cols <= 0 || rows <= 0) {
        return;
    }
    {
        std::lock_guard<std::mutex> lock(grid_mutex_);
        if (cols == cols_ && rows == rows_) {
            return;
        }
        cols_ = cols;
        rows_ = rows;
        // resize_grid() 相当の処理をロック下でインラインする
        // (resize_grid() は同じ mutex を取得するため、二重ロックを避ける)
        grid_.assign(static_cast<size_t>(cols_ * rows_), Map3DCell{});
    }
    dirty_.store(true);
}

void GodotMap3D::set_map_origin(int col, int row)
{
    origin_col_ = col;
    origin_row_ = row;
    dirty_.store(true);
}

void GodotMap3D::update_text(int x, int y, int n, uint8_t color, const char *str)
{
    if (x < 0 || !str) {
        return;
    }
    // 簡易 UTF-8 → コードポイント (1 セル = 1 コードポイント前提)
    // Hengband は cp932/UTF-8 混在で扱われるが、Phase 1 では ASCII 範囲のみ判定対象なので
    // 先頭バイトが ASCII の場合のみ抽出する。マルチバイト文字は KIND_MONSTER 扱い。
    {
        std::lock_guard<std::mutex> lock(grid_mutex_);
        if (y < 0 || y >= rows_) {
            return;
        }
        const unsigned char *p = reinterpret_cast<const unsigned char *>(str);
        int col = x;
        for (int i = 0; i < n && col < cols_; ++i) {
            char32_t cp = 0;
            if (!*p) {
                break;
            }
            if (*p < 0x80) {
                cp = *p++;
            } else if ((*p & 0xE0) == 0xC0) {
                cp = U'#'; // とりあえず壁扱い (Phase 2 で正確化)
                p += 2;
            } else if ((*p & 0xF0) == 0xE0) {
                cp = U'#';
                p += 3;
            } else {
                cp = U'?';
                ++p;
            }
            auto &cell = grid_[cell_idx(col, y)];
            // 既存のプレイヤーセルが上書きされた場合は未確定状態に戻す
            if (cell.ch == U'@' && cp != U'@' && col == player_x_ && y == player_y_) {
                player_x_ = -1;
                player_y_ = -1;
            }
            cell.ch = cp;
            cell.color = color;
            // プレイヤー位置を即座に追跡 (メッシュ再構築待ちにしない)
            if (cp == U'@' && col >= origin_col_ && y >= origin_row_) {
                player_x_ = col;
                player_y_ = y;
            }
            ++col;
        }
    }
    dirty_.store(true);
}

void GodotMap3D::wipe_cells(int x, int y, int n)
{
    {
        std::lock_guard<std::mutex> lock(grid_mutex_);
        if (y < 0 || y >= rows_) {
            return;
        }
        for (int i = 0; i < n && (x + i) < cols_; ++i) {
            const int cx = x + i;
            auto &cell = grid_[cell_idx(cx, y)];
            if (cell.ch == U'@' && cx == player_x_ && y == player_y_) {
                player_x_ = -1;
                player_y_ = -1;
            }
            cell.ch = U' ';
            cell.color = 0;
        }
    }
    dirty_.store(true);
}

void GodotMap3D::clear_all()
{
    {
        std::lock_guard<std::mutex> lock(grid_mutex_);
        for (auto &c : grid_) {
            c.ch = U' ';
            c.color = 0;
        }
        player_x_ = -1;
        player_y_ = -1;
    }
    dirty_.store(true);
}

Vector3 GodotMap3D::get_player_world_position() const
{
    std::lock_guard<std::mutex> lock(grid_mutex_);
    if (player_x_ < 0 || player_y_ < 0) {
        return Vector3(0, 0, 0);
    }
    const float wx = static_cast<float>(player_x_ - origin_col_);
    const float wz = static_cast<float>(player_y_ - origin_row_);
    return Vector3(wx, 0.0f, wz);
}

bool GodotMap3D::has_player() const
{
    std::lock_guard<std::mutex> lock(grid_mutex_);
    return player_x_ >= 0 && player_y_ >= 0;
}

void GodotMap3D::resize_grid()
{
    std::lock_guard<std::mutex> lock(grid_mutex_);
    grid_.assign(static_cast<size_t>(cols_ * rows_), Map3DCell{});
}

void GodotMap3D::rebuild_meshes()
{
    // 既存の子 MeshInstance3D をすべて削除する
    // (Phase 1 簡易実装。Phase 2 で MultiMeshInstance3D + 差分更新に置き換える)
    {
        const int child_count = get_child_count();
        for (int i = child_count - 1; i >= 0; --i) {
            Node *child = get_child(i);
            if (child) {
                remove_child(child);
                child->queue_free();
            }
        }
    }

    // グリッドのスナップショットを取る
    std::vector<Map3DCell> snapshot;
    int snap_cols, snap_rows;
    {
        std::lock_guard<std::mutex> lock(grid_mutex_);
        snapshot = grid_;
        snap_cols = cols_;
        snap_rows = rows_;
    }

    for (int gy = origin_row_; gy < snap_rows; ++gy) {
        for (int gx = origin_col_; gx < snap_cols; ++gx) {
            const auto &cell = snapshot[gy * snap_cols + gx];
            const MeshKind kind = classify_char(cell.ch);
            if (kind == KIND_NONE) {
                continue;
            }

            const MeshSpec spec = spec_for_kind(kind);
            Ref<BoxMesh> box;
            box.instantiate();
            box->set_size(Vector3(spec.size_x, spec.size_y, spec.size_z));

            Ref<StandardMaterial3D> mat;
            mat.instantiate();
            mat->set_albedo(base_color_for_kind(kind, cell.color));
            Ref<Material> mat_base = mat;

            MeshInstance3D *mi = memnew(MeshInstance3D);
            mi->set_mesh(box);
            mi->set_material_override(mat_base);
            mi->set_position(Vector3(
                static_cast<float>(gx - origin_col_),
                spec.y_offset,
                static_cast<float>(gy - origin_row_)));
            add_child(mi);
        }
    }
    // player_x_ / player_y_ は update_text / wipe_cells で常時追跡しているため
    // ここでは更新しない (再構築待ちの間にプレイヤーが移動しても遅延なく追従する)
}

void GodotMap3D::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_grid_size", "cols", "rows"),
        &GodotMap3D::set_grid_size);
    ClassDB::bind_method(D_METHOD("set_map_origin", "col", "row"),
        &GodotMap3D::set_map_origin);
    ClassDB::bind_method(D_METHOD("clear_all"), &GodotMap3D::clear_all);
    ClassDB::bind_method(D_METHOD("set_active", "active"), &GodotMap3D::set_active);
    ClassDB::bind_method(D_METHOD("is_active"), &GodotMap3D::is_active);
    ClassDB::bind_method(D_METHOD("get_player_world_position"),
        &GodotMap3D::get_player_world_position);
    ClassDB::bind_method(D_METHOD("has_player"), &GodotMap3D::has_player);
}

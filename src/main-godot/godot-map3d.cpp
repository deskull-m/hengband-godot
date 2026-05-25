/*!
 * @file godot-map3d.cpp
 * @brief Godot 3D マップ描画ノード実装 (Phase 2: floor 直読み + 差分更新)
 */

#include "godot-map3d.h"

#include <godot_cpp/classes/box_mesh.hpp>
#include <godot_cpp/classes/material.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/standard_material3d.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/vector3.hpp>

using namespace godot;
using namespace hengband_godot;

namespace {

struct MeshSpec {
    float size_x{ 0.98f };
    float size_y{ 0.05f };
    float size_z{ 0.98f };
    float y_offset{ 0.025f };
    Color color{ Color(1, 1, 1) };
};

MeshSpec spec_for_kind(uint8_t kind)
{
    MeshSpec s;
    switch (kind) {
    case M3D_FLOOR:
        s = { 1.0f, 0.05f, 1.0f, 0.025f, Color(0.30f, 0.30f, 0.32f) };
        break;
    case M3D_WALL:
        s = { 1.0f, 2.0f, 1.0f, 1.0f, Color(0.55f, 0.45f, 0.35f) };
        break;
    case M3D_DOOR_CLOSED:
        s = { 0.9f, 2.0f, 0.9f, 1.0f, Color(0.60f, 0.30f, 0.10f) };
        break;
    case M3D_DOOR_OPEN:
        s = { 0.2f, 2.0f, 0.9f, 1.0f, Color(0.40f, 0.25f, 0.10f) };
        break;
    case M3D_STAIR_UP:
        s = { 0.8f, 0.3f, 0.8f, 0.15f, Color(0.50f, 0.80f, 1.00f) };
        break;
    case M3D_STAIR_DOWN:
        s = { 0.8f, 0.3f, 0.8f, 0.15f, Color(1.00f, 0.70f, 0.20f) };
        break;
    default:
        // M3D_NONE — 呼ばれないはずだが安全側に倒す
        s = { 0.0f, 0.0f, 0.0f, 0.0f, Color(0, 0, 0, 0) };
        break;
    }
    return s;
}

constexpr int PLAYER_KIND_SENTINEL = 100;
const MeshSpec PLAYER_SPEC{ 0.6f, 1.6f, 0.6f, 0.8f, Color(1.0f, 0.2f, 0.2f) };

} // anonymous namespace

// ---------------------------------------------------------------------------
// GodotMap3D 実装
// ---------------------------------------------------------------------------

void GodotMap3D::_ready()
{
    // active_ が true なら最初に空スナップショットを反映する (= 何もしない)
    // 実際のメッシュは pending_ が届いてから生成される。
}

void GodotMap3D::_process(double delta)
{
    if (!active_.load()) {
        return;
    }
    // プレイヤー位置はクールダウンに関係なく毎フレーム追随させる
    update_player_position();

    if (rebuild_cooldown_ > 0.0) {
        rebuild_cooldown_ -= delta;
        return;
    }
    if (dirty_.exchange(false)) {
        apply_snapshot();
        rebuild_cooldown_ = REBUILD_INTERVAL;
    }
}

void GodotMap3D::set_active(bool active)
{
    const bool prev = active_.exchange(active);
    if (active && !prev) {
        // 非アクティブ→アクティブ: 強制的に差分反映を要求
        dirty_.store(true);
    } else if (!active && prev) {
        // アクティブ→非アクティブ: メッシュを全削除してメモリ解放
        clear_all_meshes();
        // current_ もクリア (再アクティブ化時に全セルが「変化」とみなされる)
        current_ = Map3DFloorSnapshot{};
    }
}

void GodotMap3D::set_floor_snapshot(int width, int height,
    const uint8_t *kinds, int player_x, int player_y)
{
    if (width <= 0 || height <= 0 || !kinds) {
        return;
    }
    const size_t n = static_cast<size_t>(width) * static_cast<size_t>(height);
    {
        std::lock_guard<std::mutex> lock(pending_mutex_);
        pending_.width = width;
        pending_.height = height;
        pending_.kinds.assign(kinds, kinds + n);
        pending_.player_x = player_x;
        pending_.player_y = player_y;
    }
    dirty_.store(true);
}

Vector3 GodotMap3D::get_player_world_position() const
{
    std::lock_guard<std::mutex> lock(pending_mutex_);
    const int px = pending_.player_x >= 0 ? pending_.player_x : current_.player_x;
    const int py = pending_.player_y >= 0 ? pending_.player_y : current_.player_y;
    if (px < 0 || py < 0) {
        return Vector3(0, 0, 0);
    }
    return Vector3(static_cast<float>(px), 0.0f, static_cast<float>(py));
}

bool GodotMap3D::has_player() const
{
    std::lock_guard<std::mutex> lock(pending_mutex_);
    if (pending_.player_x >= 0 && pending_.player_y >= 0) {
        return true;
    }
    return current_.player_x >= 0 && current_.player_y >= 0;
}

// ---------------------------------------------------------------------------
// 内部: スナップショットの差分適用
// ---------------------------------------------------------------------------

void GodotMap3D::apply_snapshot()
{
    // pending_ をスナップショットコピーする
    Map3DFloorSnapshot snap;
    {
        std::lock_guard<std::mutex> lock(pending_mutex_);
        snap = pending_;
    }

    // フロアサイズが変わったら全削除して作り直す
    if (snap.width != current_.width || snap.height != current_.height) {
        clear_all_meshes();
        current_.width = snap.width;
        current_.height = snap.height;
        current_.kinds.assign(static_cast<size_t>(snap.width)
                * static_cast<size_t>(snap.height),
            M3D_NONE);
    }

    const int w = snap.width;
    const int h = snap.height;
    if (static_cast<int>(snap.kinds.size()) != w * h) {
        return; // 不整合: スキップ
    }

    // セル単位で差分検出
    for (int dy = 0; dy < h; ++dy) {
        for (int dx = 0; dx < w; ++dx) {
            const int idx = dy * w + dx;
            const uint8_t new_kind = snap.kinds[idx];
            const uint8_t old_kind = current_.kinds[idx];
            if (new_kind == old_kind) {
                continue;
            }
            // 既存メッシュがあれば削除
            auto it = active_meshes_.find(idx);
            if (it != active_meshes_.end()) {
                MeshInstance3D *mi = it->second;
                if (mi) {
                    remove_child(mi);
                    mi->queue_free();
                }
                active_meshes_.erase(it);
            }
            // 新規メッシュを作成 (NONE 以外)
            if (new_kind != M3D_NONE) {
                MeshInstance3D *mi = create_cell_mesh(new_kind, dx, dy);
                if (mi) {
                    add_child(mi);
                    active_meshes_[idx] = mi;
                }
            }
            current_.kinds[idx] = new_kind;
        }
    }

    current_.player_x = snap.player_x;
    current_.player_y = snap.player_y;

    update_player_position();
}

MeshInstance3D *GodotMap3D::create_cell_mesh(uint8_t kind, int dx, int dy)
{
    if (kind == M3D_NONE) {
        return nullptr;
    }
    const MeshSpec spec = spec_for_kind(kind);

    Ref<BoxMesh> box;
    box.instantiate();
    box->set_size(Vector3(spec.size_x, spec.size_y, spec.size_z));

    Ref<StandardMaterial3D> mat;
    mat.instantiate();
    mat->set_albedo(spec.color);
    Ref<Material> mat_base = mat;

    MeshInstance3D *mi = memnew(MeshInstance3D);
    mi->set_mesh(box);
    mi->set_material_override(mat_base);
    mi->set_position(Vector3(
        static_cast<float>(dx),
        spec.y_offset,
        static_cast<float>(dy)));
    return mi;
}

void GodotMap3D::ensure_player_mesh()
{
    if (player_mesh_) {
        return;
    }
    Ref<BoxMesh> box;
    box.instantiate();
    box->set_size(Vector3(PLAYER_SPEC.size_x, PLAYER_SPEC.size_y, PLAYER_SPEC.size_z));

    Ref<StandardMaterial3D> mat;
    mat.instantiate();
    mat->set_albedo(PLAYER_SPEC.color);
    Ref<Material> mat_base = mat;

    player_mesh_ = memnew(MeshInstance3D);
    player_mesh_->set_mesh(box);
    player_mesh_->set_material_override(mat_base);
    add_child(player_mesh_);
}

void GodotMap3D::update_player_position()
{
    int px, py;
    {
        std::lock_guard<std::mutex> lock(pending_mutex_);
        px = pending_.player_x >= 0 ? pending_.player_x : current_.player_x;
        py = pending_.player_y >= 0 ? pending_.player_y : current_.player_y;
    }
    if (px < 0 || py < 0) {
        if (player_mesh_) {
            player_mesh_->set_visible(false);
        }
        return;
    }
    ensure_player_mesh();
    player_mesh_->set_visible(true);
    player_mesh_->set_position(Vector3(
        static_cast<float>(px),
        PLAYER_SPEC.y_offset,
        static_cast<float>(py)));
}

void GodotMap3D::clear_all_meshes()
{
    for (auto &kv : active_meshes_) {
        MeshInstance3D *mi = kv.second;
        if (mi) {
            remove_child(mi);
            mi->queue_free();
        }
    }
    active_meshes_.clear();
    if (player_mesh_) {
        remove_child(player_mesh_);
        player_mesh_->queue_free();
        player_mesh_ = nullptr;
    }
}

void GodotMap3D::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_active", "active"), &GodotMap3D::set_active);
    ClassDB::bind_method(D_METHOD("is_active"), &GodotMap3D::is_active);
    ClassDB::bind_method(D_METHOD("get_player_world_position"),
        &GodotMap3D::get_player_world_position);
    ClassDB::bind_method(D_METHOD("has_player"), &GodotMap3D::has_player);
}

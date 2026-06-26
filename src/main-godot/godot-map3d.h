#pragma once
/*!
 * @file godot-map3d.h
 * @brief Godot 3D マップ描画ノード (Phase 2: floor 直読み + 差分更新)
 *
 * Hengband のフロアデータ (FloorType.grid_array) をスナップショットとして
 * 受け取り、ダンジョン全体を Minecraft 風のボクセル 3D 表示にする。
 *
 * 設計:
 *  - ゲームスレッド (TERM_XTRA_FRESH) がフロアを走査して set_floor_snapshot を呼ぶ
 *  - GodotMap3D::_process (メインスレッド) が直前のスナップショットと比較し
 *    変化したセルだけ MeshInstance3D を追加/削除する (差分更新)
 *  - プレイヤーは単一の MeshInstance3D で常に最新位置に更新する
 *
 * セル種別:
 *   0 = NONE  (未探索 / 描画しない)
 *   1 = FLOOR
 *   2 = WALL
 *   3 = DOOR_CLOSED
 *   4 = DOOR_OPEN
 *   5 = STAIR_UP
 *   6 = STAIR_DOWN
 *
 * 座標系:
 *  - ダンジョン (dx, dy) → 3D (x, 0, z) = (dx, 0, dy)
 *  - Y 軸が上方向 (Godot 標準)
 *  - プレイヤーは (player_x_, 0, player_y_) の位置
 */

#include <atomic>
#include <cstdint>
#include <godot_cpp/classes/font.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/shader_material.hpp>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace hengband_godot {

/// セル種別 (uint8_t に格納)
enum Map3DKind : uint8_t {
    M3D_NONE = 0,
    M3D_FLOOR = 1,
    M3D_WALL = 2,
    M3D_DOOR_CLOSED = 3,
    M3D_DOOR_OPEN = 4,
    M3D_STAIR_UP = 5,
    M3D_STAIR_DOWN = 6,
    M3D_RUBBLE = 7, ///< 岩石 (STONE 持ちで壁ではない)
    M3D_VEIN = 8, ///< 鉱脈 (STONE 持ちの壁: マグマ/石英)
    M3D_TREE = 9, ///< 木
    M3D_WATER = 10, ///< 水
    M3D_LAVA = 11, ///< 溶岩
    M3D_MOUNTAIN = 12, ///< 山
    M3D_KIND_COUNT
};

/// モンスター 1 体の表示情報
struct Map3DMonster {
    int m_idx{ 0 }; ///< MonsterEntity の m_list 上の index (差分更新キー)
    int x{ 0 }; ///< ダンジョン X 座標
    int y{ 0 }; ///< ダンジョン Y 座標
    char ch{ '?' }; ///< 表示シンボル (MonraceDefinition.symbol_config.character)
    uint8_t color{ 0 }; ///< TERM_COLOR
};

/// アイテム 1 つの表示情報 (床上のもののみ。モンスター所持品は含めない)
struct Map3DItem {
    int o_idx{ 0 }; ///< ItemEntity の o_list 上の index (差分更新キー)
    int x{ 0 }; ///< ダンジョン X 座標
    int y{ 0 }; ///< ダンジョン Y 座標
    char ch{ '?' }; ///< 表示シンボル (ItemEntity::get_symbol().character)
    uint8_t color{ 0 }; ///< TERM_COLOR
};

/// フロアスナップショット
struct Map3DFloorSnapshot {
    std::vector<uint8_t> kinds; ///< height × width の Map3DKind 配列
    std::vector<Map3DMonster> monsters; ///< 視認中のモンスター一覧
    std::vector<Map3DItem> items; ///< 視認中のアイテム一覧
    int width{ 0 };
    int height{ 0 };
    int player_x{ -1 };
    int player_y{ -1 };
};

class GodotMap3D : public godot::Node3D {
    GDCLASS(GodotMap3D, godot::Node3D)

public:
    GodotMap3D() = default;
    ~GodotMap3D() override = default;

    void _ready() override;
    void _process(double delta) override;

    /// 描画アクティブ状態を切り替える (false の間は _process で何もしない)
    void set_active(bool active);
    bool is_active() const
    {
        return active_.load();
    }

    /// フロアスナップショットを設定する (ゲームスレッドから呼ばれる)
    /// kinds は width * height 個の Map3DKind 値。コピーされる。
    /// monsters / items は視認中のエンティティ配列 (空可)。コピーされる。
    void set_floor_snapshot(int width, int height,
        const uint8_t *kinds, int player_x, int player_y,
        const Map3DMonster *monsters = nullptr, int monster_count = 0,
        const Map3DItem *items = nullptr, int item_count = 0);

    /// プレイヤー位置を取得する (ワールド座標、cell 中心)
    godot::Vector3 get_player_world_position() const;

    /// プレイヤー位置が既知かどうか
    bool has_player() const;

protected:
    static void _bind_methods();

private:
    /// 直前にゲームスレッドが投入したスナップショット (pending_snapshot_mutex_ で保護)
    Map3DFloorSnapshot pending_;
    mutable std::mutex pending_mutex_;

    /// 既に描画済みのスナップショット (メインスレッドのみアクセス)
    Map3DFloorSnapshot current_;

    /// 差分更新を要求するフラグ
    std::atomic<bool> dirty_{ false };

    /// 3D 描画アクティブフラグ
    std::atomic<bool> active_{ false };

    /// 連続再生成を抑えるためのクールダウン残時間 (秒)
    double rebuild_cooldown_{ 0.0 };
    static constexpr double REBUILD_INTERVAL = 0.05;

    /// (dy * width + dx) → MeshInstance3D ポインタ (地形セル用)
    std::unordered_map<int, godot::MeshInstance3D *> active_meshes_;

    /// m_idx → MeshInstance3D ポインタ (モンスター用、差分更新キーは m_idx)
    std::unordered_map<int, godot::MeshInstance3D *> active_monsters_;

    /// 直近に描画済みのモンスター情報 (差分検出用、m_idx をキーに最新コピー)
    std::unordered_map<int, Map3DMonster> current_monsters_;

    /// o_idx → MeshInstance3D ポインタ (アイテム用、差分更新キーは o_idx)
    std::unordered_map<int, godot::MeshInstance3D *> active_items_;

    /// 直近に描画済みのアイテム情報 (差分検出用、o_idx をキーに最新コピー)
    std::unordered_map<int, Map3DItem> current_items_;

    /// モンスター / アイテムの TextMesh で使用するフォント (遅延初期化)
    godot::Ref<godot::Font> monster_font_;

    /// 壁 / 扉用の共有シェーダマテリアル (player_position uniform を毎フレーム更新)
    /// プレイヤー近辺のセルだけ半透明になり、後ろのシンボルが透けて見えるように
    /// する距離ベースのアルファフェードを実装する。
    godot::Ref<godot::ShaderMaterial> wall_material_;

    /// プレイヤー専用の単一メッシュ (半透明発光の光柱、位置マーカー)
    godot::MeshInstance3D *player_mesh_{ nullptr };

    /// プレイヤー専用の '@' シンボル TextMesh (モンスターと同じ表記)
    godot::MeshInstance3D *player_symbol_mesh_{ nullptr };

    /// pending を current に反映し、変化したセルだけ Mesh を追加/削除する
    void apply_snapshot();

    /// 指定種別のセルメッシュを作成する。kind == M3D_NONE は nullptr を返す。
    godot::MeshInstance3D *create_cell_mesh(uint8_t kind, int dx, int dy);

    /// プレイヤーメッシュを生成 (まだ無ければ)
    void ensure_player_mesh();

    /// プレイヤー位置を更新する (current_.player_x/y を反映)
    void update_player_position();

    /// モンスター差分: 新リストと current_monsters_ を比較し
    /// 移動・新規追加・消失をメッシュに反映する
    void apply_monsters(const std::vector<Map3DMonster> &new_monsters);

    /// 1 体のモンスター用 TextMesh ノードを生成する
    godot::MeshInstance3D *create_monster_mesh(const Map3DMonster &m);

    /// アイテム差分: モンスターと同様。地面に寝そべる向きで TextMesh 配置。
    void apply_items(const std::vector<Map3DItem> &new_items);

    /// 1 つのアイテム用 TextMesh ノードを生成する (地面に寝せて配置)
    godot::MeshInstance3D *create_item_mesh(const Map3DItem &it);

    /// モンスター/アイテム用フォントを必要に応じて初期化する
    void ensure_monster_font();

    /// 壁/扉用の共有シェーダマテリアルを必要に応じて初期化する。
    /// 一度初期化したものを全壁メッシュで共有する (per-cell マテリアルを作らない)。
    void ensure_wall_material();

    /// wall_material_ の player_position uniform を毎フレーム最新値に更新する。
    void update_wall_player_uniform();

    /// 全メッシュを削除する (フロアサイズ変更時)
    void clear_all_meshes();
};

} // namespace hengband_godot

# テキストマップの 3D 表示化 — 設計指針

## 1. ゴール

* メイン端末（term 0）の **マップ描画領域だけ** を、既存のテキスト/タイル描画から **Godot 3D シーン**に差し替える。
* ステータス行・メッセージ行・サブウィンドウなどの **UI は従来通りテキスト** で残す。
* テキストモード/タイルモード/3D モードの **3 択** を、既存の設定パネル（`scenes/main.tscn` の TileOption 横）に追加。

## 2. 現状把握

| レイヤ | 役割 | 実装 |
|---|---|---|
| `term_text_godot` / `term_wipe_godot` / `term_curs_godot` | テキスト描画フック | `src/main-godot/godot-term-hooks.cpp` |
| `term_pict_godot` | タイル描画フック | 同上、`GodotTileLayer::draw_tiles` を呼ぶ |
| `GodotTerminal` (Node2D) | 文字グリッド描画 | `src/main-godot/godot-terminal.{h,cpp}` |
| `GodotTileLayer` (Node2D) | タイルグリッド描画 | `src/main-godot/godot-tile-layer.{h,cpp}` |
| `TerminalPane` (.tscn) | 端末 1 個ぶんのコンテナ | `godot_project/scenes/terminal_pane.tscn` |

- マップは「term 0」の **サブ矩形**（行 1 以降、列 `COL_MAP` 〜 `panel_col_max`）に描画される。`panel_row_min/max` と `panel_col_min/max` が現在の表示範囲。
- `pict_hook` 経由で **タイル ID（fg/bg 各 row/col）** が 1 セル単位で届くため、これを **ボクセル種別** にマップすれば 3D 化に必要な情報は出揃う。
- C++ 側からは `system/floor/floor-info.h` を介して `Grid`（地形・モンスター・アイテム・視認フラグ）に直接アクセス可能。**`pict_hook` 経由よりこちらの方が高情報量**。

## 3. アーキテクチャ方針

### 3.1 「マップ領域だけ」3D に差し替える

`TerminalPane` の SubViewport の中に、**テキスト/タイル層と並べて 3D 専用の SubViewport を追加**し、マップ矩形ピクセル領域に重ねる：

```
TerminalPane
└── SubViewportContainer (2D)
    └── SubViewport
        ├── TileLayer       (既存)
        ├── Terminal        (既存)
        └── Map3DOverlay    ← 新規: TextureRect で SubViewport3D を表示
              └── SubViewport(3D) → World3D + Camera3D + ボクセルメッシュ
```

利点:
- 端末スクロール・リサイズ・フォント変更といった既存挙動を壊さない。
- メイン端末 0 のみ 3D、サブ端末は素のままという制御が `terminal_index == 0` の分岐で済む。
- 設定で OFF にすると `Map3DOverlay.visible = false` で即座にテキスト/タイル表示に戻る。

### 3.2 データソースの二択

| 案 | 入力 | 長所 | 短所 |
|---|---|---|---|
| **A. pict_hook 流用** | `(ap,cp,tap,tcp)` の row/col | 既存パスをそのまま使える | タイル ID→ ボクセル種の対応表が必要、視認外/記憶のみ表現が間接的 |
| **B. floor 直読み** | `Grid` / `TerrainType` / `MonsterEntity` 等 | 種別・高さ・視認状態・モンスター/アイテムを正確に取得 | C++ 側のスレッド境界とロック設計が必要 |

**推奨は B**。`pict_hook` は描画通知としては使い、**実データはゲームスレッドから周期的にスナップショット**して 3D 側に渡す。Phase 1 では A で素早く可視化 → Phase 2 で B に切り替えるとリスクが小さい。

### 3.3 スレッド境界

既存と同じ `std::mutex` ベースで、`MapSnapshot { width, height, cells[], player_x, player_y, depth }` を C++ で保持し、`GodotMap3D::poll_snapshot()` から取得する。
- 書き手: ゲームスレッド（`lite_spot` / `update_view` のタイミング、または毎ターン末）。
- 読み手: Godot メインスレッド（`_process` で差分検出してメッシュ更新）。

## 4. 3D 表現の最小仕様

### 4.1 セル → ボクセルマッピング

| Hengband 地形 | 3D 表現 | テクスチャ案 |
|---|---|---|
| 床 (floor) | 高さ 0 の床ブロック | 既存 8x8/16x16 タイルの floor 部分を流用 |
| 壁 (wall, magma, quartz) | 高さ 2〜3 の壁ボクセル | 既存 `wall.bmp` パターン |
| 扉 (door open/closed) | 高さ 2、開放/閉鎖でメッシュ差分 | 専用 or タイル流用 |
| 階段 (up/down) | 高さ 0.5 のステップ + マーカー | 矢印デカール |
| 水/溶岩 | 高さ 0.3 のアニメ平面 | シェーダで簡易波 |
| 闇/未探索 | **メッシュ非生成**（黒のまま） | — |
| 記憶のみ (memorized) | 彩度を落としたマテリアル | シェーダ uniform |

### 4.2 モンスター/アイテム/プレイヤー

- いずれも **タイル画像をビルボード化（QuadMesh + AlphaCutoff）** して該当ボクセルの上に立てるのが最も実装コストが低い。
- プレイヤーは **三人称俯瞰** で固定し、Camera3D は (player_x, player_y) を追従。 **斜め見下ろし 45° / FOV 60°** から開始。
- 一人称切替は将来オプションとして TODO に残す。

### 4.3 メッシュ生成戦略

- 初期実装は **セルあたり 1 MeshInstance3D** で十分動くが、ダンジョン全体（最大約 198×66 ≒ 13k セル）では重い。
- 早期に **MultiMeshInstance3D**（種別ごとに 1 つ）に移行。視認/記憶セルだけ転送する。
- 変更検出は「セル属性 + 視認状態」のハッシュ比較で OK。`lite_spot` 起点のフラグで部分更新できればさらに良い。

## 5. 設定 UI 追加

`scenes/main.tscn` の `ConfigPanel/PanelVBox` に行を追加：

* `MapModeRow` ＝ `Label "マップ表示"` + `OptionButton`（`テキスト / タイル / 3D(ボクセル)`）
* 永続化は `GameState.map_mode: int` を追加して `user://` の config に保存。
* 切替時の挙動:
  * テキスト/タイル: 既存どおり。`Map3DOverlay.visible = false`、必要に応じて `apply_tile_mode(...)` 呼出し。
  * 3D: `apply_tile_mode(false, "ascii")` でテキストモードに固定（裏でテキストグリッドは更新される）、`Map3DOverlay.visible = true`。

## 6. 実装フェーズ（推奨順）

### Phase 1 — スパイク（〜数日）
1. `Map3DOverlay` シーン雛形（SubViewport + Camera3D + DirectionalLight + 仮の床メッシュ）を作成し、メイン端末のマップ矩形に重ねるだけの動作確認。
2. `GodotMap3D` C++ ノード（`Node3D` 派生 GDExtension）を新設し、`pict_hook` から `(x,y, fg/bg)` をスナップショットに記録。
3. `floor` か `wall` かの 2 値だけ判定して 1 セル = 1 BoxMesh を生成。プレイヤー位置に赤いキューブを置く。

**ゴール**: 「マップが立体になっている」と分かる状態。

### Phase 2 — データ精度向上
1. データソースを **floor 直読み** に切り替え（`PlayerType::current_floor_ptr` の `Grid` を mutex 越しにコピー）。
2. ボクセル種別を §4.1 に従って拡張。視認/記憶/暗闇の 3 状態を区別。
3. MultiMeshInstance3D へリファクタ。

### Phase 3 — エンティティとカメラ
1. モンスター/アイテム/プレイヤーのビルボード化（既存タイルセットの該当行/列を AtlasTexture で参照）。
2. カメラをプレイヤー追従に。回転キーは当面なし（Hengband は方向感が固定のため）。
3. テクスチャを既存 `8x8.bmp` / `16x16.bmp` から自動抽出（`GodotTileLayer` のロード処理を共有化）。

### Phase 4 — 仕上げ
1. 設定 UI からのモード切替と永続化。
2. ターゲット指定（`*` キー等）時のカーソル表現を 3D に重ねる。
3. パフォーマンス計測（最低 60fps / 198×66 マップ）と、視錐台外カリング/メモ化の追加。

## 7. リスクと注意点

* **タイル ID 体系の信頼性**: `pict_hook` の `(ap,cp,tap,tcp)` の意味は `ANGBAND_GRAF`（"old"/"new"/"ne2"）で変わる。Phase 1 は最小限の判定にとどめ、Phase 2 で floor 直読みに移行する前提。
* **スレッド安全性**: `floor.grid_array` をゲームスレッド以外から直接読むのは危険。**必ずスナップショット**を取り、Godot 側からは読み取り専用にする。既存の `s_graphics_mode_changed` と同じ流儀の `std::atomic` フラグで「再スナップショット要」を伝えるとよい。
* **既存テスト/CI への影響**: 3D 関連クラスを追加しても、テキストモード/タイルモードの既存パスは変更しない（追加のみ）方針で進める。
* **i18n**: 設定 UI の文言は既存 UI と同じく日本語固定で OK（コードは UTF-8）。
* **ファイルパスとアセット**: 当面はテクスチャを新規追加せず既存 `lib/xtra/graf/*.bmp` を流用。新規アセットが必要になったら `godot_project/assets/3d/` 配下に置く。

## 8. 成果物のディレクトリ案

```
godot_project/
  scenes/
    map3d_overlay.tscn       (新規)
  scripts/
    map3d_overlay.gd         (新規 — シーン管理・モード切替)
src/main-godot/
  godot-map3d.{h,cpp}        (新規 — Node3D / GDExtension)
  godot-map-snapshot.{h,cpp} (新規 — スレッド境界のスナップショット)
docs/
  3d-map-plan.md             (このドキュメント)
```

## 9. 受け入れ基準（Phase 1 完了時）

* 設定パネルに「マップ表示」モード切替が存在する。
* 3D モード選択で、メイン端末のマップ矩形が 3D 表示になる。
* テキスト/タイルに戻すと完全に元の挙動。
* 既存のキー入力・セーブ/ロード・サブウィンドウは無影響。
* 60fps を割らない（標準 80×24 ダンジョン階で測定）。

extends SubViewportContainer

## Map3DOverlay
##
## メイン端末ペインの「マップ矩形」(列 COL_MAP=12 以降, 行 ROW_MAP=1 以降) に
## 重ねて表示する 3D 描画オーバーレイ。
##
## - 内部に SubViewport(3D) を持ち、Camera3D / DirectionalLight3D / GodotMap3D を配置する。
## - 親 (terminal_pane.gd) が cell サイズと grid サイズに応じて
##   `position` / `size` を毎フレーム調整する。

## Hengband 標準のマップ矩形原点 (windows/main-window-util.h より)
const COL_MAP: int = 13
const ROW_MAP: int = 1

## カメラの俯瞰角度 (45° ≒ Minecraft 風アイソメ)
const CAMERA_PITCH_DEG: float = 55.0

## プレイヤーキャラ追従時のカメラからプレイヤーまでの距離 (セル単位)
const CAMERA_DISTANCE: float = 16.0

## カメラがプレイヤーを追従するかどうか
@export var follow_player: bool = true

func _ready() -> void:
	# 親が SubViewport のサイズを直接設定するため stretch を有効化
	stretch = true
	# 入力は通過させる (テキスト端末側がキー入力を受け取る)
	mouse_filter = Control.MOUSE_FILTER_IGNORE

func _process(_delta: float) -> void:
	if not follow_player or not visible:
		return
	var map3d: GodotMap3D = $SubViewport/Map3D as GodotMap3D
	if map3d == null:
		return
	var camera: Camera3D = $SubViewport/Camera3D
	if camera == null:
		return
	# プレイヤー未検出のときは現状維持 (起動直後など)
	if not map3d.has_player():
		return
	_aim_camera_at(camera, map3d.get_player_world_position())

## カメラを target セルに向けて配置する (固定俯瞰角度)
## look_at_from_position で position と回転を一括設定する。
## (camera.transform.origin = ... は GDScript ではローカル struct コピーになり
##  反映されないことがあるため避ける)
func _aim_camera_at(camera: Camera3D, target: Vector3) -> void:
	var pitch_rad := deg_to_rad(CAMERA_PITCH_DEG)
	var horiz := CAMERA_DISTANCE * cos(pitch_rad)
	var vert := CAMERA_DISTANCE * sin(pitch_rad)
	# プレイヤー (target) の斜め後ろ上方から見下ろす
	var eye := target + Vector3(0.0, vert, horiz)
	# 注視点は target のやや上 (床より少し高め) にして見やすくする
	var look := target + Vector3(0.0, 0.4, 0.0)
	camera.look_at_from_position(eye, look, Vector3.UP)

## 親ペインから呼ばれる: SubViewport のピクセルサイズ要求
## stretch=true のとき SubViewportContainer が自動でリサイズするため何もしない。
## 互換性のためにシグネチャだけ残す。
func resize_viewport(_px_size: Vector2i) -> void:
	pass

## 親ペインから呼ばれる: 3D マップノードを取得する (HengbandGame.register_map3d 用)
func get_map3d_node() -> Node:
	return $SubViewport/Map3D

extends SubViewportContainer
class_name Map3DOverlay

## Map3DOverlay
##
## メイン端末ペインの「マップ矩形」(列 COL_MAP=13 以降, 行 ROW_MAP=1 以降) に
## 重ねて表示する 3D 描画オーバーレイ。
##
## - 内部に SubViewport(3D) を持ち、Camera3D / DirectionalLight3D / GodotMap3D を配置する。
## - 親 (terminal_pane.gd) が cell サイズと grid サイズに応じて
##   `position` / `size` を毎フレーム調整する。

## Hengband 標準のマップ矩形原点 (windows/main-window-util.h より)
const COL_MAP: int = 13
const ROW_MAP: int = 1

## カメラの俯瞰角度 (45°)
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
	if not follow_player:
		return
	var map3d: GodotMap3D = $SubViewport/Map3D as GodotMap3D
	if map3d == null:
		return
	var camera: Camera3D = $SubViewport/Camera3D
	if camera == null:
		return
	var target: Vector3 = map3d.get_player_world_position()
	# プレイヤー未検出のときは現状維持
	if target == Vector3.ZERO and not _is_player_known(map3d):
		return
	_aim_camera_at(camera, target)

## カメラを target セルに向けて配置する (固定俯瞰角度)
func _aim_camera_at(camera: Camera3D, target: Vector3) -> void:
	var pitch_rad := deg_to_rad(CAMERA_PITCH_DEG)
	var horiz := CAMERA_DISTANCE * cos(pitch_rad)
	var vert := CAMERA_DISTANCE * sin(pitch_rad)
	# 斜め後ろ上方からプレイヤーを見下ろす (Z+ 方向後退、Y+ 上昇)
	var eye := target + Vector3(0.0, vert, horiz)
	camera.transform.origin = eye
	camera.look_at(target + Vector3(0.0, 0.4, 0.0), Vector3.UP)

func _is_player_known(map3d: GodotMap3D) -> bool:
	# get_player_world_position は未検出時に Vector3.ZERO を返す。
	# 原点 (= ROW_MAP, COL_MAP) も Vector3.ZERO に重なるが、Phase 1 では割り切る。
	return map3d.get_player_world_position() != Vector3.ZERO

## 親ペインから呼ばれる: SubViewport のピクセルサイズを更新する
func resize_viewport(px_size: Vector2i) -> void:
	var sv: SubViewport = $SubViewport
	if sv == null:
		return
	sv.size = Vector2i(maxi(64, px_size.x), maxi(64, px_size.y))

## 親ペインから呼ばれる: マップ矩形の原点を 3D マップノードに反映する
func set_map_origin(col: int, row: int) -> void:
	var map3d: GodotMap3D = $SubViewport/Map3D as GodotMap3D
	if map3d:
		map3d.set_map_origin(col, row)

## 親ペインから呼ばれる: 3D マップノードを取得する (HengbandGame.register_map3d 用)
func get_map3d_node() -> Node:
	return $SubViewport/Map3D

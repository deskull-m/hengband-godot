extends PanelContainer

## TerminalPane: 分割レイアウト用のターミナルペインシーン
##
## ヘッダバー（タイトル + 縦追加/横追加/閉じるボタン）と
## SubViewport を持つ GodotTerminal コンテナ。
## 分割・クローズの要求は親の main.gd にシグナルで伝達する。

signal v_split_requested(pane: Node)  ## 縦に追加（左右に並べる）
signal h_split_requested(pane: Node)  ## 横に追加（上下に並べる）
signal close_requested(pane: Node)    ## このペインを閉じる

## ゲームノードから割り当てられたターミナルインデックス
var terminal_index: int = -1
var _game_node: Node = null
## _do_split でペインが親から外れて再追加されると Godot 4.6 は _ready() を再呼び出しする。
## このフラグでシグナルの二重接続を防ぐ。
var _initialized: bool = false

## ペイン固有のフォント設定（空文字/0 = グローバル設定を使用）
var pane_font_name: String = ""
var pane_font_size: int = 0

## マップ 3D オーバーレイの表示状態 (term 0 でのみ意味を持つ)
var map_3d_enabled: bool = false

## Hengband 標準のマップ矩形原点 (src/window/main-window-util.h: COL_MAP=13, ROW_MAP=1)
const COL_MAP: int = 13
const ROW_MAP: int = 1
## マップ矩形下端の余白行数 (メッセージ行・ステータス行を残すための余白)
const ROW_MAP_BOTTOM_MARGIN: int = 1

func _ready() -> void:
	if _initialized:
		return
	_initialized = true
	$PaneVBox/Header/HBox/VSplitButton.pressed.connect(func(): v_split_requested.emit(self))
	$PaneVBox/Header/HBox/HSplitButton.pressed.connect(func(): h_split_requested.emit(self))
	$PaneVBox/Header/HBox/CloseButton.pressed.connect(func(): close_requested.emit(self))
	$PaneVBox/SubViewportContainer.resized.connect(fit_subviewport)
	add_to_group("terminal_panes")

## ターミナルを初期化してゲームノードに登録する。
## add_child() の後に呼ぶこと（ノードがシーンツリーに入ってから）。
func setup(game: Node, term_idx: int) -> void:
	terminal_index = term_idx
	_game_node = game

	$PaneVBox/Header/HBox/TitleLabel.text = "Main" if term_idx == 0 else "Sub %d" % term_idx
	$PaneVBox/Header/HBox/CloseButton.visible = (term_idx != 0)
	_apply_header_color(term_idx == 0)

	var term = $PaneVBox/SubViewportContainer/SubViewport/TerminalContainer/Terminal
	var tiles = $PaneVBox/SubViewportContainer/SubViewport/TerminalContainer/TileLayer
	game.register_terminal(term_idx, term, tiles)

	# メインペイン (term 0) のみ 3D マップノードを登録する
	if term_idx == 0:
		var overlay := _get_map3d_overlay()
		if overlay:
			var map3d_node: Node = overlay.get_map3d_node()
			if map3d_node and game.has_method("register_map3d"):
				game.register_map3d(term_idx, map3d_node)

func _apply_header_color(is_main: bool) -> void:
	var style := StyleBoxFlat.new()
	style.content_margin_left = 2.0
	style.content_margin_top = 0.0
	style.content_margin_right = 2.0
	style.content_margin_bottom = 0.0
	# Main: 下部バーと同色、Sub: やや薄い同系色
	style.bg_color = Color(0.16, 0.07, 0.07, 0.95) if is_main else Color(0.22, 0.10, 0.10, 0.80)
	$PaneVBox/Header.add_theme_stylebox_override("panel", style)

## このペインのターミナルにフォントを直接適用してグリッドを再フィットする
func apply_pane_font(font: Font, size: int) -> void:
	var term = $PaneVBox/SubViewportContainer/SubViewport/TerminalContainer/Terminal
	if not term:
		return
	term.set_terminal_font(font, size)
	fit_subviewport()

## SubViewport のサイズをコンテナのピクセルサイズに合わせ、
## グリッドサイズをセルサイズで割った値で更新する。
func fit_subviewport() -> void:
	var svc: SubViewportContainer = $PaneVBox/SubViewportContainer
	var term = $PaneVBox/SubViewportContainer/SubViewport/TerminalContainer/Terminal
	if not svc or not term:
		return
	var px := Vector2i(int(svc.size.x), int(svc.size.y))
	if px.x <= 0 or px.y <= 0:
		return
	# stretch=true 時は SubViewportContainer が sv.size を自動管理するため手動設定不要
	if _game_node == null or terminal_index < 0:
		return
	var cw: int = term.get_cell_width()
	var ch: int = term.get_cell_height()
	if cw <= 0 or ch <= 0:
		return
	_game_node.set_sub_window_size(terminal_index, maxi(1, px.x / cw), maxi(1, px.y / ch))

	# Phase 1: メインペインの 3D オーバーレイを map 矩形に合わせて位置決めする
	if terminal_index == 0:
		_fit_map3d_overlay(px, cw, ch)

## 3D オーバーレイをマップ矩形のピクセル範囲に合わせて配置する
## SubViewport のサイズは SubViewportContainer (stretch=true) が自動管理するため
## ここでは Control の position / size のみを更新する。
func _fit_map3d_overlay(px_size: Vector2i, cell_w: int, cell_h: int) -> void:
	var overlay := _get_map3d_overlay()
	if overlay == null:
		return
	var origin := Vector2(COL_MAP * cell_w, ROW_MAP * cell_h)
	var size := Vector2(
		maxf(0.0, px_size.x - origin.x),
		maxf(0.0, px_size.y - origin.y - ROW_MAP_BOTTOM_MARGIN * cell_h)
	)
	overlay.position = origin
	overlay.size = size

## 3D オーバーレイの表示/非表示を切り替える
## 有効時:
##  - 3D ビューポートを表示 / アクティブ化
##  - Terminal の背景黒塗りを止め、self_modulate.a を 0.5 にして 3D を透かす
##  - TileLayer も併せて 50% 半透明化
func set_map_3d_enabled(enabled: bool) -> void:
	map_3d_enabled = enabled
	var overlay := _get_map3d_overlay()
	if overlay:
		overlay.visible = enabled
		var map3d_node: Node = overlay.get_map3d_node()
		if map3d_node and map3d_node.has_method("set_active"):
			map3d_node.set_active(enabled)

	var term: Node = $PaneVBox/SubViewportContainer/SubViewport/TerminalContainer/Terminal
	if term and term.has_method("set_transparent_mode"):
		term.set_transparent_mode(enabled, 0.5)

	# TileLayer も併せて半透明化 (タイルモード + 3D の組み合わせ用)
	var tiles: CanvasItem = $PaneVBox/SubViewportContainer/SubViewport/TerminalContainer/TileLayer
	if tiles:
		var mod := tiles.self_modulate
		mod.a = 0.5 if enabled else 1.0
		tiles.self_modulate = mod

func _get_map3d_overlay() -> Map3DOverlay:
	return get_node_or_null("PaneVBox/SubViewportContainer/SubViewport/Map3DOverlay")

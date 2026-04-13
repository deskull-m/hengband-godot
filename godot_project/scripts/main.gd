extends Node

## Hengband メインエントリポイント (Phase 7)
## HengbandGame GDExtension ノードが start_game() でゲームスレッドを起動する

## lib/ ディレクトリのパス (空文字列 = 実行ファイルの隣を自動検出)
## Godot エディタでテストする場合: プロジェクトルートから見た lib/ の絶対パスを指定
@export var game_lib_path: String = ""

## フォントプリセット一覧（最後の "カスタム" は手入力用）
const FONT_PRESETS: Array[String] = [
	"MS Gothic",
	"Yu Gothic",
	"BIZ UDGothic",
	"Meiryo",
	"Noto Sans Mono CJK JP",
	"Consolas",
	"Courier New",
	"カスタム",
]

## タイルモード選択肢: インデックスが GameState.tile_mode に対応
const TILE_ITEMS: Array[String] = ["なし", "8×8.bmp", "16×16.bmp"]

## 解決済み lib/ パス（タイル読み込みなどで再利用する）
var _lib_path: String = ""

func _ready() -> void:
	# ウィンドウ × ボタンを自前で処理するため自動終了を無効化する
	get_tree().set_auto_accept_quit(false)

	GameState.load_config()

	var game := $HengbandGame
	if not game:
		push_error("HengbandGame node not found")
		return

	_apply_font(game)

	# ウィンドウリサイズ時にターミナルグリッドを追従させる
	get_viewport().size_changed.connect(_on_viewport_size_changed)
	# 初期サイズでグリッドをフィット (start_game 前に確定させる)
	_fit_main_term()

	_setup_config_ui()

	# lib_path が未設定の場合はプロジェクトディレクトリの隣の lib/ を使う
	_lib_path = game_lib_path
	if _lib_path.is_empty():
		_lib_path = ProjectSettings.globalize_path("res://../lib")

	print("Hengband: lib_path = ", _lib_path)
	print("Hengband: save_path = ", GameState.save_path)
	game.start_game(_lib_path, GameState.save_path)

	# 起動時にタイル設定を適用する
	_apply_tiles(game)

func _notification(what: int) -> void:
	if what == NOTIFICATION_WM_CLOSE_REQUEST:
		var game := $HengbandGame
		if game and game.is_game_started():
			# ゲームが起動中 → セーブ＆終了をキューに注入する。
			# ESC×2 で現在の画面から抜け、Ctrl+X で終了ダイアログを開き、
			# 'y' で確定する。
			var ih := $HengbandGame/InputHandler
			ih.inject_key(0x1B)  # ESC
			ih.inject_key(0x1B)  # ESC
			ih.inject_key(0x18)  # Ctrl+X (Save & Quit)
			ih.inject_key(0x79)  # 'y' (確認)
			# 実際の終了は Hengband の godot_quit_aux が tree->quit() を呼ぶことで行われる
		else:
			# ゲーム未起動（タイトル画面など）はそのまま終了
			get_tree().quit()

func _on_viewport_size_changed() -> void:
	_fit_main_term()

## SubViewport をウィンドウサイズに合わせ、ターミナルグリッドを最大化する
func _fit_main_term() -> void:
	var game := $HengbandGame
	var sub_vp: SubViewport = $GameViewport/SubViewport
	if not game or not sub_vp:
		return

	var win_size := Vector2i(get_viewport().get_visible_rect().size)
	if win_size.x <= 0 or win_size.y <= 0:
		return

	sub_vp.size = win_size
	game.fit_term_to_viewport(win_size)

## SystemFont を生成してゲームに適用し、セルサイズ変化に合わせてグリッドを再フィットする
func _apply_font(game: Node) -> void:
	var font := SystemFont.new()
	font.font_names = [GameState.font_name, "Courier New", "Monospace"]
	game.set_game_font(font, GameState.font_size)
	_fit_main_term()

## タイル設定を適用する。失敗時は tile_mode を 0 に戻して UI を更新する
func _apply_tiles(game: Node) -> void:
	match GameState.tile_mode:
		1:
			var tileset := _lib_path + "/xtra/graf/8x8.bmp"
			var mask := _lib_path + "/xtra/graf/8x8a.bmp"
			var ok: bool = game.load_tileset(tileset, mask, 8, 8, "old")
			if not ok:
				push_warning("タイルセットの読み込みに失敗しました: " + tileset)
				GameState.tile_mode = 0
				_sync_tile_ui()
			else:
				game.set_bigtile_enabled(GameState.use_bigtile)
		2:
			var tileset := _lib_path + "/xtra/graf/16x16.bmp"
			var mask := _lib_path + "/xtra/graf/mask.bmp"
			var ok: bool = game.load_tileset(tileset, mask, 16, 16, "new")
			if not ok:
				push_warning("タイルセットの読み込みに失敗しました: " + tileset)
				GameState.tile_mode = 0
				_sync_tile_ui()
			else:
				game.set_bigtile_enabled(GameState.use_bigtile)
		_:
			game.set_tile_rendering_enabled(false)
			game.set_bigtile_enabled(false)

## TileOption と BigtileCheck を GameState.tile_mode に同期する
func _sync_tile_ui() -> void:
	var tile_option: OptionButton = $ConfigLayer/ConfigPanel/PanelVBox/TileRow/TileOption
	tile_option.select(GameState.tile_mode)
	var bigtile_check: CheckBox = $ConfigLayer/ConfigPanel/PanelVBox/BigtileRow/BigtileCheck
	bigtile_check.disabled = (GameState.tile_mode == 0)

## コンフィグ UI の初期化とシグナル接続
func _setup_config_ui() -> void:
	var font_option: OptionButton = $ConfigLayer/ConfigPanel/PanelVBox/FontRow/FontOption
	for preset in FONT_PRESETS:
		font_option.add_item(preset)

	var font_edit: LineEdit = $ConfigLayer/ConfigPanel/PanelVBox/FontNameRow/FontNameEdit
	font_edit.text = GameState.font_name

	var spin: SpinBox = $ConfigLayer/ConfigPanel/PanelVBox/FontSizeRow/FontSizeSpinBox
	spin.value = GameState.font_size

	var tile_option: OptionButton = $ConfigLayer/ConfigPanel/PanelVBox/TileRow/TileOption
	for item in TILE_ITEMS:
		tile_option.add_item(item)
	tile_option.selected = GameState.tile_mode

	var bigtile_check: CheckBox = $ConfigLayer/ConfigPanel/PanelVBox/BigtileRow/BigtileCheck
	bigtile_check.button_pressed = GameState.use_bigtile
	bigtile_check.disabled = (GameState.tile_mode == 0)

	# 現在のフォント名にマッチするプリセットを選択（なければ "カスタム"）
	var idx := FONT_PRESETS.find(GameState.font_name)
	if idx < 0:
		idx = FONT_PRESETS.size() - 1
	font_option.selected = idx
	_sync_font_edit_from_preset(idx)

	$ConfigLayer/GearButton.pressed.connect(_on_gear_pressed)
	font_option.item_selected.connect(_on_font_preset_selected)
	font_edit.text_changed.connect(_on_font_name_changed)
	spin.value_changed.connect(_on_font_size_changed)
	tile_option.item_selected.connect(_on_tile_option_selected)
	bigtile_check.toggled.connect(_on_bigtile_check_toggled)
	$ConfigLayer/ConfigPanel/PanelVBox/ButtonRow/ApplyButton.pressed.connect(_on_apply_config)
	$ConfigLayer/ConfigPanel/PanelVBox/ButtonRow/CloseButton.pressed.connect(_on_close_config)

func _on_gear_pressed() -> void:
	var panel: Panel = $ConfigLayer/ConfigPanel
	panel.visible = not panel.visible

## プリセット選択時: FontNameEdit に反映してリアルタイム適用
func _on_font_preset_selected(idx: int) -> void:
	_sync_font_edit_from_preset(idx)
	_apply_config_live()

## カスタム以外はプリセット名を FontNameEdit に設定して読み取り専用にする
func _sync_font_edit_from_preset(idx: int) -> void:
	var font_edit: LineEdit = $ConfigLayer/ConfigPanel/PanelVBox/FontNameRow/FontNameEdit
	if idx < FONT_PRESETS.size() - 1:
		font_edit.text = FONT_PRESETS[idx]
		font_edit.editable = false
	else:
		font_edit.editable = true

## フォント名を直接編集したときにリアルタイム適用
func _on_font_name_changed(_text: String) -> void:
	_apply_config_live()

## フォントサイズを変更したときにリアルタイム適用
func _on_font_size_changed(_value: float) -> void:
	_apply_config_live()

## タイル選択ドロップダウンが変化したときにリアルタイム適用
func _on_tile_option_selected(idx: int) -> void:
	GameState.tile_mode = idx
	var bigtile_check: CheckBox = $ConfigLayer/ConfigPanel/PanelVBox/BigtileRow/BigtileCheck
	bigtile_check.disabled = (idx == 0)
	_apply_tiles($HengbandGame)
	if $HengbandGame.is_game_started():
		$HengbandGame/InputHandler.inject_key(0x12)  # Ctrl+R: 画面再描画

func _on_bigtile_check_toggled(toggled: bool) -> void:
	GameState.use_bigtile = toggled
	$HengbandGame.set_bigtile_enabled(toggled)
	if $HengbandGame.is_game_started():
		$HengbandGame/InputHandler.inject_key(0x12)  # Ctrl+R: 再描画で bigtile 反映

## UI の現在値を GameState に反映してフォントを即時適用する（保存はしない）
func _apply_config_live() -> void:
	var font_edit: LineEdit = $ConfigLayer/ConfigPanel/PanelVBox/FontNameRow/FontNameEdit
	var spin: SpinBox = $ConfigLayer/ConfigPanel/PanelVBox/FontSizeRow/FontSizeSpinBox
	GameState.font_name = font_edit.text
	GameState.font_size = int(spin.value)
	_apply_font($HengbandGame)

## 設定を保存してパネルを閉じる
func _on_apply_config() -> void:
	_apply_config_live()
	GameState.save_config()
	$ConfigLayer/ConfigPanel.visible = false

func _on_close_config() -> void:
	$ConfigLayer/ConfigPanel.visible = false

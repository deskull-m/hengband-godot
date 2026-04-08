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

func _ready() -> void:
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
	var lib_path := game_lib_path
	if lib_path.is_empty():
		lib_path = ProjectSettings.globalize_path("res://../lib")

	print("Hengband: lib_path = ", lib_path)
	print("Hengband: save_path = ", GameState.save_path)
	game.start_game(lib_path, GameState.save_path)

func _notification(what: int) -> void:
	if what == NOTIFICATION_WM_CLOSE_REQUEST:
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

## SystemFont を生成してゲームに適用する
func _apply_font(game: Node) -> void:
	var font := SystemFont.new()
	font.font_names = [GameState.font_name, "Courier New", "Monospace"]
	game.set_game_font(font, GameState.font_size)

## コンフィグ UI の初期化とシグナル接続
func _setup_config_ui() -> void:
	var option: OptionButton = $ConfigLayer/ConfigPanel/PanelVBox/FontRow/FontOption
	for preset in FONT_PRESETS:
		option.add_item(preset)

	var font_edit: LineEdit = $ConfigLayer/ConfigPanel/PanelVBox/FontNameRow/FontNameEdit
	font_edit.text = GameState.font_name

	var spin: SpinBox = $ConfigLayer/ConfigPanel/PanelVBox/FontSizeRow/FontSizeSpinBox
	spin.value = GameState.font_size

	# 現在のフォント名にマッチするプリセットを選択（なければ "カスタム"）
	var idx := FONT_PRESETS.find(GameState.font_name)
	if idx < 0:
		idx = FONT_PRESETS.size() - 1
	option.selected = idx
	_on_font_preset_changed(idx)

	$ConfigLayer/GearButton.pressed.connect(_on_gear_pressed)
	option.item_selected.connect(_on_font_preset_changed)
	$ConfigLayer/ConfigPanel/PanelVBox/ButtonRow/ApplyButton.pressed.connect(_on_apply_config)
	$ConfigLayer/ConfigPanel/PanelVBox/ButtonRow/CloseButton.pressed.connect(_on_close_config)

func _on_gear_pressed() -> void:
	var panel: Panel = $ConfigLayer/ConfigPanel
	panel.visible = not panel.visible

## プリセット選択時: プリセット名を FontNameEdit に反映し、カスタム以外は編集不可にする
func _on_font_preset_changed(idx: int) -> void:
	var font_edit: LineEdit = $ConfigLayer/ConfigPanel/PanelVBox/FontNameRow/FontNameEdit
	if idx < FONT_PRESETS.size() - 1:
		font_edit.text = FONT_PRESETS[idx]
		font_edit.editable = false
	else:
		font_edit.editable = true

## 設定を適用して保存し、パネルを閉じる
func _on_apply_config() -> void:
	var font_edit: LineEdit = $ConfigLayer/ConfigPanel/PanelVBox/FontNameRow/FontNameEdit
	var spin: SpinBox = $ConfigLayer/ConfigPanel/PanelVBox/FontSizeRow/FontSizeSpinBox
	GameState.font_name = font_edit.text
	GameState.font_size = int(spin.value)
	GameState.save_config()
	_apply_font($HengbandGame)
	$ConfigLayer/ConfigPanel.visible = false

func _on_close_config() -> void:
	$ConfigLayer/ConfigPanel.visible = false

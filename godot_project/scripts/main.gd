extends Node

## Hengband メインエントリポイント (Phase 7)
## HengbandGame GDExtension ノードが start_game() でゲームスレッドを起動する

## lib/ ディレクトリのパス (空文字列 = 実行ファイルの隣を自動検出)
## Godot エディタでテストする場合: プロジェクトルートから見た lib/ の絶対パスを指定
@export var game_lib_path: String = ""

func _ready() -> void:
	var game := $HengbandGame
	if not game:
		push_error("HengbandGame node not found")
		return

	# 日本語対応モノスペースフォントを設定
	# MS Gothic / Yu Gothic はWindows標準の等幅日本語フォント
	var font := SystemFont.new()
	font.font_names = ["MS Gothic", "Yu Gothic", "Meiryo", "Courier New", "Monospace"]
	game.set_game_font(font, 16)

	# ウィンドウリサイズ時にターミナルグリッドを追従させる
	get_viewport().size_changed.connect(_on_viewport_size_changed)
	# 初期サイズでグリッドをフィット (start_game 前に確定させる)
	_fit_main_term()

	# lib_path が未設定の場合はプロジェクトディレクトリの隣の lib/ を使う
	var lib_path := game_lib_path
	if lib_path.is_empty():
		# res:// → プロジェクトフォルダ → ../lib/
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

	# ウィンドウの表示可能領域 (OS ウィンドウサイズ) を取得
	var win_size := Vector2i(get_viewport().get_visible_rect().size)
	if win_size.x <= 0 or win_size.y <= 0:
		return

	# SubViewport をウィンドウと同サイズにすることで 1:1 ピクセル描画を維持する
	sub_vp.size = win_size

	# ターミナルグリッドを新ピクセルサイズに合わせて再計算
	game.fit_term_to_viewport(win_size)

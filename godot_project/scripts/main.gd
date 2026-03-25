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

	# lib_path が未設定の場合はプロジェクトディレクトリの隣の lib/ を使う
	var lib_path := game_lib_path
	if lib_path.is_empty():
		# res:// → プロジェクトフォルダ → ../lib/
		lib_path = ProjectSettings.globalize_path("res://../lib")

	print("Hengband: lib_path = ", lib_path)
	game.start_game(lib_path)

func _notification(what: int) -> void:
	if what == NOTIFICATION_WM_CLOSE_REQUEST:
		get_tree().quit()

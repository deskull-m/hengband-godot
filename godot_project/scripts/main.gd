extends Node

## Hengband メインエントリポイント
## ゲームロジックは別スレッドで実行し、Godot メインスレッドをブロックしない

var _game_thread: Thread

func _ready() -> void:
	print("Hengband GDExtension: initializing...")
	# TODO: Phase 7 で HengbandGame GDExtension ノードを追加し
	#       _game_thread で start_game() を呼び出す
	# _game_thread = Thread.new()
	# _game_thread.start(_run_game)

func _run_game() -> void:
	# HengbandGame.start_game() を呼び出す予定
	pass

func _notification(what: int) -> void:
	if what == NOTIFICATION_WM_CLOSE_REQUEST:
		_on_close_requested()

func _on_close_requested() -> void:
	if _game_thread and _game_thread.is_started():
		_game_thread.wait_to_finish()
	get_tree().quit()

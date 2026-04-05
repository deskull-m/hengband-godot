extends Control

## タイトル画面スクリプト
## 新規ゲーム / セーブデータから始める を選択してゲーム画面に遷移する

func _ready() -> void:
	$ButtonContainer/VBox/NewGameButton.pressed.connect(_on_new_game)
	$ButtonContainer/VBox/LoadGameButton.pressed.connect(_on_load_game)
	$SaveFileDialog.file_selected.connect(_on_save_file_selected)

func _on_new_game() -> void:
	GameState.save_path = ""
	get_tree().change_scene_to_file("res://scenes/main.tscn")

func _on_load_game() -> void:
	$SaveFileDialog.popup_centered()

func _on_save_file_selected(path: String) -> void:
	GameState.save_path = path
	get_tree().change_scene_to_file("res://scenes/main.tscn")

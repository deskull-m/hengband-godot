extends Control

## タイトル画面スクリプト
## 新規ゲーム / セーブデータから始める を選択してゲーム画面に遷移する

func _ready() -> void:
	_load_title_image()
	get_viewport().size_changed.connect(_fit_title_image)
	$ButtonContainer/VBox/NewGameButton.pressed.connect(_on_new_game)
	$ButtonContainer/VBox/LoadGameButton.pressed.connect(_on_load_game)
	$SaveFileDialog.file_selected.connect(_on_save_file_selected)

func _load_title_image() -> void:
	# lib/xtra/graf/hengband_title.png を実行時に動的ロードする
	var img_path := ProjectSettings.globalize_path("res://../image/hengband_title.png")
	if not FileAccess.file_exists(img_path):
		push_warning("hengband_title.png not found: " + img_path)
		return
	var img := Image.load_from_file(img_path)
	if img == null:
		push_warning("Failed to load image: " + img_path)
		return
	$Background.texture = ImageTexture.create_from_image(img)
	# レイアウト確定後に1フレーム遅らせてサイズ適用する
	call_deferred("_fit_title_image")

func _fit_title_image() -> void:
	var tex: Texture2D = $Background.texture
	if tex == null:
		return
	var vp_size := get_viewport().get_visible_rect().size
	var target_w := vp_size.x * 0.7
	var aspect := float(tex.get_width()) / float(tex.get_height())
	var target_h := target_w / aspect
	# テクスチャ由来の最小サイズ制約を解除してから適用
	$Background.custom_minimum_size = Vector2.ZERO
	$Background.set_size(Vector2(target_w, target_h))
	$Background.set_position(Vector2((vp_size.x - target_w) * 0.5, vp_size.y * 0.1))

func _on_new_game() -> void:
	GameState.save_path = ""
	get_tree().change_scene_to_file("res://scenes/main.tscn")

func _on_load_game() -> void:
	$SaveFileDialog.popup_centered()

func _on_save_file_selected(path: String) -> void:
	GameState.save_path = path
	get_tree().change_scene_to_file("res://scenes/main.tscn")

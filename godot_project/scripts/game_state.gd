extends Node

## ゲーム状態の Autoload シングルトン
## タイトル画面からゲーム画面へのデータ受け渡し、および設定の永続化に使用する

## セーブファイルのパス（空文字列 = 新規ゲーム）
var save_path: String = ""

## フォント名（SystemFont の font_names[0] に使用）
var font_name: String = "MS Gothic"

## フォントサイズ (pt)
var font_size: int = 16

## タイル表示を使用するかどうか
var use_tiles: bool = false

const _CONFIG_PATH := "user://hengband_config.cfg"

func save_config() -> void:
	var cfg := ConfigFile.new()
	cfg.set_value("display", "font_name", font_name)
	cfg.set_value("display", "font_size", font_size)
	cfg.set_value("display", "use_tiles", use_tiles)
	cfg.save(_CONFIG_PATH)

func load_config() -> void:
	var cfg := ConfigFile.new()
	if cfg.load(_CONFIG_PATH) != OK:
		return
	font_name = cfg.get_value("display", "font_name", font_name)
	font_size = cfg.get_value("display", "font_size", font_size)
	use_tiles = cfg.get_value("display", "use_tiles", use_tiles)

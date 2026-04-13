extends Node

## ゲーム状態の Autoload シングルトン
## タイトル画面からゲーム画面へのデータ受け渡し、および設定の永続化に使用する

## セーブファイルのパス（空文字列 = 新規ゲーム）
var save_path: String = ""

## フォント名（SystemFont の font_names[0] に使用）
var font_name: String = "MS Gothic"

## フォントサイズ (pt)
var font_size: int = 16

## タイル表示モード: 0=なし / 1=8×8 / 2=16×16
var tile_mode: int = 0

## タイルを2倍幅で表示するかどうか
var use_bigtile: bool = false

const _CONFIG_PATH := "user://hengband_config.cfg"

func save_config() -> void:
	var cfg := ConfigFile.new()
	cfg.set_value("display", "font_name", font_name)
	cfg.set_value("display", "font_size", font_size)
	cfg.set_value("display", "tile_mode", tile_mode)
	cfg.set_value("display", "use_bigtile", use_bigtile)
	cfg.save(_CONFIG_PATH)

func load_config() -> void:
	var cfg := ConfigFile.new()
	if cfg.load(_CONFIG_PATH) != OK:
		return
	font_name = cfg.get_value("display", "font_name", font_name)
	font_size = cfg.get_value("display", "font_size", font_size)
	# 旧設定 use_tiles (bool) からの移行
	var old_use_tiles: bool = cfg.get_value("display", "use_tiles", false)
	tile_mode = cfg.get_value("display", "tile_mode", 1 if old_use_tiles else 0)
	use_bigtile = cfg.get_value("display", "use_bigtile", use_bigtile)

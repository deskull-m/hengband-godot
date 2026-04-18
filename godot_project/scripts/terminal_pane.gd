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

func _ready() -> void:
	$PaneVBox/Header/VSplitButton.pressed.connect(func(): v_split_requested.emit(self))
	$PaneVBox/Header/HSplitButton.pressed.connect(func(): h_split_requested.emit(self))
	$PaneVBox/Header/CloseButton.pressed.connect(func(): close_requested.emit(self))
	$PaneVBox/SubViewportContainer.resized.connect(fit_subviewport)
	add_to_group("terminal_panes")

## ターミナルを初期化してゲームノードに登録する。
## add_child() の後に呼ぶこと（ノードがシーンツリーに入ってから）。
func setup(game: Node, term_idx: int) -> void:
	terminal_index = term_idx
	_game_node = game

	$PaneVBox/Header/TitleLabel.text = "Main" if term_idx == 0 else "Sub %d" % term_idx
	$PaneVBox/Header/CloseButton.visible = (term_idx != 0)

	var term = $PaneVBox/SubViewportContainer/SubViewport/TerminalContainer/Terminal
	var tiles = $PaneVBox/SubViewportContainer/SubViewport/TerminalContainer/TileLayer
	game.register_terminal(term_idx, term, tiles)

## SubViewport のサイズをコンテナのピクセルサイズに合わせ、
## グリッドサイズをセルサイズで割った値で更新する。
func fit_subviewport() -> void:
	var svc: SubViewportContainer = $PaneVBox/SubViewportContainer
	var sv: SubViewport = $PaneVBox/SubViewportContainer/SubViewport
	var term = $PaneVBox/SubViewportContainer/SubViewport/TerminalContainer/Terminal
	if not svc or not sv or not term:
		return
	var px := Vector2i(int(svc.size.x), int(svc.size.y))
	if px.x <= 0 or px.y <= 0:
		return
	sv.size = px
	if _game_node == null or terminal_index < 0:
		return
	var cw: int = term.get_cell_width()
	var ch: int = term.get_cell_height()
	if cw <= 0 or ch <= 0:
		return
	_game_node.set_sub_window_size(terminal_index, maxi(1, px.x / cw), maxi(1, px.y / ch))

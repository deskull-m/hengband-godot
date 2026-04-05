# 変愚蛮怒 Godot版 (Hengband × Godot Engine)

[![Build](https://github.com/deskull-m/hengband-godot/actions/workflows/create-release.yml/badge.svg)](https://github.com/deskull-m/hengband-godot/actions)

> **オリジナル変愚蛮怒のゲームガイドは [docs/readme-hengband-original.md](docs/readme-hengband-original.md) を参照してください。**

---

## 概要

本リポジトリは [変愚蛮怒 (Hengband)](https://hengband.github.io/) の **Godot Engine 移植版**です。
Win32 GDI / WinMM に依存していた描画・音声・入力処理を [Godot 4.x GDExtension](https://docs.godotengine.org/en/stable/tutorials/scripting/gdextension/) に置き換え、ゲームロジック (`src/` 以下) はそのまま維持します。

### アーキテクチャ

```
┌──────────────────────────────────────────────┐
│               Godot Engine                   │
│  ┌──────────┐  ┌──────────┐  ┌───────────┐  │
│  │ Rendering│  │  Audio   │  │   Input   │  │
│  │  (GPU)   │  │(AudioBus)│  │(InputEvent│  │
│  └────┬─────┘  └────┬─────┘  └─────┬─────┘  │
│       └─────────────┴───────────────┘        │
│                GDExtension API               │
│  ┌───────────────────────────────────────┐   │
│  │  src/main-godot/  (C++ バックエンド)   │   │
│  │  ・term_type フック実装                │   │
│  │  ・GodotTerminal / GodotTileLayer      │   │
│  │  ・GodotInputHandler                   │   │
│  │  ・GodotAudioManager                   │   │
│  └──────────────┬────────────────────────┘   │
│                 │ 直接関数呼び出し             │
│  ┌──────────────┴────────────────────────┐   │
│  │  Hengband ゲームロジック (C++20)       │   │
│  │  src/ 以下の既存コードをそのまま使用   │   │
│  └───────────────────────────────────────┘   │
└──────────────────────────────────────────────┘
```

ゲームロジックは Godot メインスレッドとは **別スレッド** (`Thread`) で実行し、UI スレッドをブロックしません。

---

## 必要環境

| ツール | バージョン | 用途 |
|---|---|---|
| [Godot Engine](https://godotengine.org/) | 4.3 以上 | エディタ / 実行 |
| [SCons](https://scons.org/) | 4.x 以上 | ビルドシステム |
| MSVC (Visual Studio 2022) | v143 以上 | C++ コンパイラ (Windows) |
| Python | 3.8 以上 | SCons の実行環境 |

---

## ビルド手順

### 1. リポジトリのクローン

```bash
git clone https://github.com/deskull-m/hengband-godot.git
cd hengband-godot
git submodule update --init godot-cpp
```

### 2. GDExtension ライブラリのビルド

```bash
# デバッグビルド (Godot エディタでの実行に使用)
scons platform=windows target=template_debug

# リリースビルド
scons platform=windows target=template_release
```

ビルド成果物は `bin/` に出力されます。

```
bin/
  hengband.windows.template_debug.x86_64.dll
  hengband.windows.template_debug.x86_64.pdb
```

### 3. Godot プロジェクトの起動

Godot エディタで `godot_project/project.godot` を開いて実行します。

> **注意:** `lib/` ディレクトリ（変愚蛮怒のデータファイル群）が `godot_project/` の隣に配置されている必要があります。

---

## ディレクトリ構成

```
hengband-godot/
├── src/
│   ├── main-godot/          # Godot バックエンド (本移植の核心)
│   │   ├── hengband-gdextension.h/.cpp   # HengbandGame GDExtension ノード
│   │   ├── godot-terminal.h/.cpp         # テキスト描画 (term_type フック)
│   │   ├── godot-tile-layer.h/.cpp       # タイル描画 (pict_hook)
│   │   ├── godot-input-handler.h/.cpp    # キーボード・マウス入力
│   │   ├── godot-audio-manager.h/.cpp    # 音楽・効果音
│   │   ├── godot-term-hooks.h/.cpp       # term_type フック実装
│   │   └── godot-init.h/.cpp             # ゲーム初期化 (WinMain 相当)
│   └── ...                  # 既存 Hengband ゲームロジック (変更なし)
├── godot_project/
│   ├── project.godot
│   ├── scenes/
│   │   ├── title.tscn        # タイトル画面
│   │   └── main.tscn         # ゲーム画面
│   ├── scripts/
│   │   ├── game_state.gd     # Autoload シングルトン (シーン間データ共有)
│   │   ├── title.gd          # タイトル画面ロジック
│   │   └── main.gd           # ゲーム画面ロジック
│   └── assets/
│       └── hengband_title.png
├── godot-cpp/               # godot-cpp サブモジュール
├── bin/                     # ビルド成果物 (.dll)
├── lib/                     # 変愚蛮怒 データファイル群
├── SConstruct               # ビルドスクリプト
└── docs/
    └── readme-hengband-original.md  # オリジナル変愚蛮怒 README
```

---

## Godot 版固有の仕様

### タイトル画面

起動時にタイトル画像 (`hengband_title.png`) を表示し、以下を選択します。

- **新規ゲーム** — キャラクター作成から開始
- **セーブデータから始める** — Godot の `FileDialog` でセーブファイル (`.sav`) を選択してロード

選択内容は `GameState` Autoload シングルトン (`GameState.save_path`) を通じてゲーム画面に渡されます。

### 文字エンコーディング

| 区分 | 設定 |
|---|---|
| ソースファイル | UTF-8 (`/source-charset:utf-8`) |
| 実行時文字セット | CP932 (日本語 Windows デフォルト) |
| Godot 内文字列 | UTF-8 (`godot::String`) |

ゲームロジック内部は CP932 のまま動作させ、Godot に渡す際に変換します。

### キーボード入力

`pref-win.prf` のマクロトリガー定義を使用します (`ANGBAND_SYS = "win"`)。
方向キーやファンクションキーは `^_xHH\r` 形式のエスケープシーケンスに変換して `term_key_push()` に渡します。

Ctrl+A〜Z は Windows + Godot の組み合わせで `get_unicode()` が 0 を返すことがあるため、キーコードベースのフォールバック処理を実装しています。

### ターミナルグリッドの自動リサイズ

ウィンドウをリサイズすると、フォントのセルサイズに基づいてターミナルの行数・列数が自動的に再計算されます (`HengbandGame::fit_term_to_viewport()`)。

### ビルド環境の分離

godot-cpp と Hengband ソースでは必要な MSVC フラグが異なるため、SCons 環境をクローンして別々に管理しています。

| 環境 | 用途 | 特記フラグ |
|---|---|---|
| `env` (godot-cpp 標準) | GDExtension リンク | `/utf-8` |
| `hengband_env` (クローン) | Hengband ソース | `/source-charset:utf-8` のみ (`/utf-8` を除去) |

---

## 実装済みフェーズ

| フェーズ | 内容 | 状態 |
|---|---|---|
| Phase 1 | GDExtension 基盤構築 / SCons セットアップ | 完了 |
| Phase 2 | テキスト描画バックエンド | 完了 |
| Phase 3 | タイル描画バックエンド | 完了 |
| Phase 4 | キーボード入力処理 | 完了 |
| Phase 5 | 音声・BGM バックエンド | 完了 |
| Phase 6 | ウィンドウ管理 / FileDialog | 完了 |
| Phase 7 | タイトル画面 / セーブデータ選択 / 統合 | 実装中 |

---

## 関連リンク

- [変愚蛮怒 公式サイト](https://hengband.github.io/)
- [オリジナル hengband リポジトリ](https://github.com/hengband/hengband)
- [Godot GDExtension ドキュメント](https://docs.godotengine.org/en/stable/tutorials/scripting/gdextension/)

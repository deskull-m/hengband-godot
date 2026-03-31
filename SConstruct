#!/usr/bin/env python
"""
Hengband GDExtension ビルドスクリプト
Usage:
  scons                          # デフォルトビルド (platform=windows target=template_debug)
  scons platform=linux           # Linux向けビルド
  scons target=template_release  # リリースビルド
"""

import os
import sys

# godot-cpp の SConstruct を読み込む
env = SConscript("godot-cpp/SConstruct")

# --- Hengband ソースファイル設定 ---

# main-godot バックエンドのソースファイル
godot_backend_sources = Glob("src/main-godot/*.cpp")

# Hengband ゲームコア (main-win を除く全ソース)
def collect_hengband_sources():
    sources = []
    exclude_dirs = {
        "main-win",    # Windows バックエンド (今回は除外)
        "main-unix",   # Unix バックエンド (今回は除外)
        "main-godot",  # Godot バックエンド (godot_backend_sources で別途追加)
        "net",         # libcurl 依存のネットワーク機能 (Godot 版では不要)
        "test",        # 単体テスト (各自 main() を持つため DLL ビルドから除外)
    }
    exclude_files = {
        "main-x11.cpp",
        "main-gcu.cpp",
        "main-cap.cpp",
        "main.cpp",      # Unix エントリポイント (WinMain の代わりに main-godot を使用)
        "main-win.cpp",  # Windows ネイティブバックエンド (GDExtension では不要)
    }
    for root, dirs, files in os.walk("src"):
        # 除外ディレクトリをスキップ
        dirs[:] = [d for d in dirs if d not in exclude_dirs]
        for f in files:
            if f.endswith(".cpp") and f not in exclude_files:
                sources.append(os.path.join(root, f).replace("\\", "/"))
    return sources

hengband_core_sources = collect_hengband_sources()

# --- ビルド設定 ---

env.Append(CPPPATH=[
    "src",
    "src/external-lib",
    "src/external-lib/include",  # fmtlib, tl::optional 等
])

env.Append(CPPDEFINES=["USE_GODOT"])

# C++20 を要求
if env["platform"] == "windows":
    # h-config.h は WIN32 マクロで WINDOWS を判定するが、
    # x86_64 MSVC は _WIN32 のみ定義するため明示的に追加
    env.Append(CPPDEFINES=["WINDOWS", "JP", "SJIS"])
    # fmtlib v11 の consteval フォーマット文字列検証を無効化
    # (_() マクロがランタイム文字列を返すため compile-time check 不可)
    env.Append(CPPDEFINES=["FMT_USE_CONSTEVAL=0"])
    # C++ 例外ハンドラ有効化 (MSVC 既定では無効)
    env.Append(CXXFLAGS=["/std:c++20", "/EHsc"])
    # timeGetTime (record-play-movie.cpp) と DbgHelp (stack-trace) に必要
    env.Append(LIBS=["winmm", "DbgHelp"])
else:
    env.Append(CXXFLAGS=["-std=c++20"])

# --- ライブラリのビルド ---

# fmtlib のコンパイル済み実装 (ヘッダオンリーではなくコンパイルモードで使用)
fmt_sources = ["src/external-lib/fmt/format.cc"]

all_sources = godot_backend_sources + hengband_core_sources + fmt_sources

library = env.SharedLibrary(
    "godot_project/../bin/hengband{}{}".format(
        env["suffix"],
        env["SHLIBSUFFIX"]
    ),
    source=all_sources,
)

Default(library)

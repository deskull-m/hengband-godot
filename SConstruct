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
    }
    exclude_files = {
        "main-x11.cpp",
        "main-gcu.cpp",
        "main-cap.cpp",
        "main.cpp",    # Unix エントリポイント (WinMain の代わりに main-godot を使用)
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
])

env.Append(CPPDEFINES=["USE_GODOT"])

# C++20 を要求
if env["platform"] == "windows":
    env.Append(CXXFLAGS=["/std:c++20"])
else:
    env.Append(CXXFLAGS=["-std=c++20"])

# --- ライブラリのビルド ---

all_sources = godot_backend_sources + hengband_core_sources

library = env.SharedLibrary(
    "godot_project/../bin/hengband{}{}".format(
        env["suffix"],
        env["SHLIBSUFFIX"]
    ),
    source=all_sources,
)

Default(library)

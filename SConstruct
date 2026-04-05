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

# CPPPATH と USE_GODOT は全ソースに共通 (godot-cpp との互換性に問題なし)
env.Append(CPPPATH=[
    "src",
    "src/external-lib",
    "src/external-lib/include",  # fmtlib, tl::optional 等
])

env.Append(CPPDEFINES=["USE_GODOT"])

# --- Hengband 専用ビルド環境 ---
# godot-cpp は CCFLAGS に /utf-8 (=/source-charset:utf-8 /execution-charset:utf-8) を追加する。
# Hengband は z-term の SJIS モデルに合わせて /execution-charset:932 が必要なため、
# env をクローンして /utf-8 を除去し独自のチャーセットフラグを設定する。
hengband_env = env.Clone()

if env["platform"] == "windows":
    # h-config.h は WIN32 マクロで WINDOWS を判定するが、
    # x86_64 MSVC は _WIN32 のみ定義するため明示的に追加
    hengband_env.Append(CPPDEFINES=["WINDOWS", "JP", "SJIS"])
    # fmtlib v11 の consteval フォーマット文字列検証を無効化
    # (_() マクロがランタイム文字列を返すため compile-time check 不可)
    hengband_env.Append(CPPDEFINES=["FMT_USE_CONSTEVAL=0"])
    # /utf-8 は /source-charset:utf-8 /execution-charset:utf-8 の短縮形。
    # /source-charset:utf-8 と同時指定できないため除去して個別に指定する。
    hengband_env["CCFLAGS"] = [f for f in hengband_env.get("CCFLAGS", []) if f != "/utf-8"]
    # /source-charset:utf-8  : ソースファイルを UTF-8 として読む
    # /execution-charset は省略 → システムデフォルト (日本語 Windows = CP932) を使用
    #   → z-term の 1バイト/セル・SJIS モデルと一致させるため必須
    hengband_env.Append(CCFLAGS=["/source-charset:utf-8"])
    # godot-cpp が /std:c++17 を追加するため除去してから /std:c++20 を設定する
    hengband_env["CXXFLAGS"] = [f for f in hengband_env.get("CXXFLAGS", []) if f not in ("/std:c++17", "/std:c++20")]
    # C++ 例外ハンドラ有効化 (MSVC 既定では無効)
    hengband_env.Append(CXXFLAGS=["/std:c++20", "/EHsc"])
    # timeGetTime (record-play-movie.cpp) と DbgHelp (stack-trace) に必要
    # LIBS はリンクステップで使われるため、SharedLibrary を呼ぶ env に追加する
    env.Append(LIBS=["winmm", "DbgHelp"])
else:
    hengband_env.Append(CXXFLAGS=["-std=c++20"])

# --- ライブラリのビルド ---

# fmtlib のコンパイル済み実装 (ヘッダオンリーではなくコンパイルモードで使用)
fmt_sources = ["src/external-lib/fmt/format.cc"]

# Hengband 専用環境でオブジェクトファイルを生成
hengband_objects = hengband_env.SharedObject(
    list(godot_backend_sources) + hengband_core_sources + fmt_sources
)

library = env.SharedLibrary(
    "godot_project/../bin/hengband{}{}".format(
        env["suffix"],
        env["SHLIBSUFFIX"]
    ),
    source=hengband_objects,
)

Default(library)

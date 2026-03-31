/*!
 * @file hengband-gdextension.cpp
 * @brief Hengband GDExtension 実装 + エントリポイント
 */

#include "hengband-gdextension.h"

#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>

using namespace godot;
using namespace hengband_godot;

// --- HengbandGame 実装 ---

void HengbandGame::_ready()
{
    // TODO: Phase 7 でゲームスレッドを起動する
    // 現在は Phase 1 のスタブ実装
}

void HengbandGame::_process(double delta)
{
    // TODO: Phase 2〜6 でフック呼び出しキューの処理を実装する
    (void)delta;
}

void HengbandGame::_notification(int p_what)
{
    if (p_what == NOTIFICATION_WM_CLOSE_REQUEST) {
        // TODO: ゲームスレッドへの終了シグナル送信
    }
}

void HengbandGame::start_game()
{
    // TODO: Phase 7 で play_game() 呼び出しを実装する
}

void HengbandGame::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("start_game"), &HengbandGame::start_game);
}

// --- GDExtension エントリポイント ---

void initialize_hengband_module(ModuleInitializationLevel p_level)
{
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
        return;
    }
    ClassDB::register_class<HengbandGame>();
}

void uninitialize_hengband_module(ModuleInitializationLevel p_level)
{
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
        return;
    }
}

extern "C" {

GDExtensionBool GDE_EXPORT hengband_gdextension_init(
    GDExtensionInterfaceGetProcAddress p_get_proc_address,
    const GDExtensionClassLibraryPtr p_library,
    GDExtensionInitialization *r_initialization)
{
    godot::GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);
    init_obj.register_initializer(initialize_hengband_module);
    init_obj.register_terminator(uninitialize_hengband_module);
    init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);
    return init_obj.init();
}

} // extern "C"

#pragma once

#include <windows.h>

namespace reshade::api
{
    struct effect_runtime;
}

namespace arknights
{
    enum class shortcut_action : unsigned int
    {
        move_up,
        move_down,
        move_left,
        move_right,
        skip,
        gacha_skip,
        confirm_yes,
        story_hide_ui,
        story_auto,
        story_auto_speed,
        story_continue,
        story_history,
        fullscreen,
        count,
    };

    void load_shortcut_settings() noexcept;
    [[nodiscard]] bool shortcut_matches(
        shortcut_action action,
        UINT key) noexcept;
    void preserve_overlay_scale_after_resize(HWND game_window) noexcept;
    void cancel_shortcut_capture() noexcept;
    void draw_shortcut_settings(reshade::api::effect_runtime *runtime);
}

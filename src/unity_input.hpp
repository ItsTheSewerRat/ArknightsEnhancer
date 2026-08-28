#pragma once

#include <windows.h>

namespace arknights
{
    [[nodiscard]] bool initialize_unity_input() noexcept;
    [[nodiscard]] bool trigger_skip() noexcept;
    [[nodiscard]] bool trigger_gacha_skip() noexcept;
    [[nodiscard]] bool trigger_confirm_yes() noexcept;
    [[nodiscard]] bool trigger_story_hide_ui() noexcept;
    [[nodiscard]] bool trigger_story_auto() noexcept;
    [[nodiscard]] bool trigger_story_auto_speed() noexcept;
    [[nodiscard]] bool trigger_story_continue() noexcept;
    [[nodiscard]] bool trigger_story_history() noexcept;
    [[nodiscard]] bool drag_unity_ui(
        HWND window,
        POINT origin_client,
        POINT target_client) noexcept;
}

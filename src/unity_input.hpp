#pragma once

#include <windows.h>

namespace arknights
{
    [[nodiscard]] bool initialize_unity_input() noexcept;
    [[nodiscard]] bool click_unity_ui(
        HWND window,
        POINT target_client) noexcept;
    [[nodiscard]] bool drag_unity_ui(
        HWND window,
        POINT origin_client,
        POINT target_client) noexcept;
}

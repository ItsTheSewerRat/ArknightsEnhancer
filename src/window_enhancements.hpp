#pragma once

#include <windows.h>

namespace arknights
{
    void install_window_enhancements(HWND game_window, HMODULE module) noexcept;
    void notify_window_presented() noexcept;
    void preview_game_window_move(int window_x, int window_y) noexcept;
    void uninstall_window_enhancements() noexcept;
    void request_window_enhancements_shutdown() noexcept;
}

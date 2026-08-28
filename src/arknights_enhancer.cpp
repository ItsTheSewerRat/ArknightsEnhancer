// Operator-positioning behavior based on https://github.com/ACK72/THRM-EX

#include <windows.h>
#include <windowsx.h>

#include <imgui.h>
#include <reshade.hpp>

#include "shortcut_settings.hpp"
#include "unity_input.hpp"
#include "window_enhancements.hpp"

#include <array>
#include <atomic>
#include <cstdint>
#include <cstdlib>

namespace
{
    constexpr UINT k_cancel_drag_message = WM_APP + 0x5A1;
    constexpr ULONGLONG k_position_hook_delay_ms = 5000;
    constexpr LONG k_drag_threshold_pixels = 3;

    struct input_state
    {
        HWND window = nullptr;
        HHOOK message_hook = nullptr;
        HHOOK position_hook = nullptr;
        DWORD window_thread_id = 0;
        reshade::api::effect_runtime *runtime = nullptr;
        ULONGLONG position_hook_ready_at = 0;

        bool tracking_physical_drag = false;
        bool physical_drag_moved = false;
        POINT physical_drag_start = {};

        bool anchor_valid = false;
        POINT anchor = {};

        UINT active_key = 0;
        std::array<bool, 256> consumed_direction_keys = {};
        UINT action_key_down = 0;
        UINT fullscreen_key_down = 0;
    };

    input_state g_input;
    HMODULE g_module = nullptr;
    std::atomic_bool g_reshade_overlay_open = false;

    enum class direction : std::uint8_t
    {
        none = 0,
        up = 1u << 0,
        down = 1u << 1,
        left = 1u << 2,
        right = 1u << 3,
    };

    [[nodiscard]] direction direction_from_key(UINT key) noexcept
    {
        if (arknights::shortcut_matches(
                arknights::shortcut_action::move_up,
                key))
            return direction::up;
        if (arknights::shortcut_matches(
                arknights::shortcut_action::move_down,
                key))
            return direction::down;
        if (arknights::shortcut_matches(
                arknights::shortcut_action::move_left,
                key))
            return direction::left;
        if (arknights::shortcut_matches(
                arknights::shortcut_action::move_right,
                key))
            return direction::right;
        return direction::none;
    }

    [[nodiscard]] POINT point_from_message(LPARAM value) noexcept
    {
        return POINT { GET_X_LPARAM(value), GET_Y_LPARAM(value) };
    }

    void consume_message(MSG &message) noexcept
    {
        message.message = WM_NULL;
        message.wParam = 0;
        message.lParam = 0;
    }

    [[nodiscard]] POINT clamp_to_client(POINT point, const RECT &client) noexcept
    {
        if (point.x < client.left) point.x = client.left;
        if (point.y < client.top) point.y = client.top;
        if (point.x >= client.right) point.x = client.right - 1;
        if (point.y >= client.bottom) point.y = client.bottom - 1;
        return point;
    }

    [[nodiscard]] bool action_shortcut_matches(UINT key) noexcept
    {
        return arknights::shortcut_matches(
                   arknights::shortcut_action::skip,
                   key) ||
               arknights::shortcut_matches(
                   arknights::shortcut_action::gacha_skip,
                   key) ||
               arknights::shortcut_matches(
                   arknights::shortcut_action::confirm_yes,
                   key);
    }

    void trigger_context_action(UINT key) noexcept
    {
        if (arknights::shortcut_matches(
                arknights::shortcut_action::confirm_yes,
                key) &&
            arknights::trigger_confirm_yes())
        {
            return;
        }
        if (arknights::shortcut_matches(
                arknights::shortcut_action::gacha_skip,
                key) &&
            arknights::trigger_gacha_skip())
        {
            return;
        }
        if (arknights::shortcut_matches(
                arknights::shortcut_action::skip,
                key))
        {
            static_cast<void>(arknights::trigger_skip());
        }
    }

    void clear_active_direction() noexcept
    {
        g_input.active_key = 0;
        g_input.consumed_direction_keys.fill(false);
    }

    void finish_active_direction() noexcept
    {
        clear_active_direction();
    }

    [[nodiscard]] bool begin_direction_drag(MSG &message, UINT key, direction facing) noexcept
    {
        if (!g_input.anchor_valid || g_input.window == nullptr)
            return false;

        RECT client = {};
        if (!GetClientRect(g_input.window, &client))
            return false;

        const LONG width = client.right - client.left;
        const LONG height = client.bottom - client.top;
        if (width <= 0 || height <= 0)
            return false;

        const POINT origin = clamp_to_client(g_input.anchor, client);
        POINT target = origin;
        const LONG horizontal_distance = MulDiv(width, 15, 100);
        const LONG vertical_distance = MulDiv(height, 15, 100);

        switch (facing)
        {
        case direction::up: target.y -= vertical_distance; break;
        case direction::down: target.y += vertical_distance; break;
        case direction::left: target.x -= horizontal_distance; break;
        case direction::right: target.x += horizontal_distance; break;
        case direction::none: return false;
        }

        target = clamp_to_client(target, client);
        if (!arknights::drag_unity_ui(g_input.window, origin, target))
            return false;

        g_input.active_key = key;
        consume_message(message);
        return true;
    }

    void track_physical_mouse_message(const MSG &message) noexcept
    {
        const POINT point = point_from_message(message.lParam);

        switch (message.message)
        {
        case WM_LBUTTONDOWN:
            g_input.tracking_physical_drag = true;
            g_input.physical_drag_moved = false;
            g_input.physical_drag_start = point;
            g_input.anchor_valid = false;
            break;

        case WM_MOUSEMOVE:
            if (g_input.tracking_physical_drag && (message.wParam & MK_LBUTTON) != 0)
            {
                const LONG delta_x = std::labs(point.x - g_input.physical_drag_start.x);
                const LONG delta_y = std::labs(point.y - g_input.physical_drag_start.y);
                if (delta_x >= k_drag_threshold_pixels || delta_y >= k_drag_threshold_pixels)
                    g_input.physical_drag_moved = true;
            }
            break;

        case WM_LBUTTONUP:
            if (g_input.tracking_physical_drag)
            {
                g_input.anchor_valid = g_input.physical_drag_moved;
                if (g_input.anchor_valid)
                    g_input.anchor = point;
            }
            g_input.tracking_physical_drag = false;
            g_input.physical_drag_moved = false;
            break;

        default:
            break;
        }
    }

    void process_game_message(MSG &message) noexcept
    {
        if (message.message == k_cancel_drag_message)
        {
            finish_active_direction();
            g_input.action_key_down = 0;
            g_input.fullscreen_key_down = 0;
            consume_message(message);
            return;
        }

        if (message.message == WM_KILLFOCUS ||
            (message.message == WM_ACTIVATEAPP && message.wParam == FALSE))
        {
            finish_active_direction();
            g_input.tracking_physical_drag = false;
            g_input.anchor_valid = false;
            g_input.action_key_down = 0;
            g_input.fullscreen_key_down = 0;
            return;
        }

        const bool overlay_open = g_reshade_overlay_open.load(std::memory_order_relaxed);
        if (!overlay_open &&
            (message.message == WM_LBUTTONDOWN ||
             message.message == WM_MOUSEMOVE ||
             message.message == WM_LBUTTONUP))
        {
            if (g_input.active_key == 0)
                track_physical_mouse_message(message);
            return;
        }

        if (overlay_open ||
            (message.message != WM_KEYDOWN && message.message != WM_KEYUP &&
             message.message != WM_SYSKEYDOWN && message.message != WM_SYSKEYUP))
        {
            return;
        }

        const UINT key = static_cast<UINT>(message.wParam);
        const bool key_down = message.message == WM_KEYDOWN || message.message == WM_SYSKEYDOWN;

        if (arknights::shortcut_matches(
                arknights::shortcut_action::fullscreen,
                key))
        {
            const bool modified = (GetKeyState(VK_CONTROL) & 0x8000) != 0 ||
                                  (GetKeyState(VK_MENU) & 0x8000) != 0 ||
                                  (GetKeyState(VK_SHIFT) & 0x8000) != 0 ||
                                  (GetKeyState(VK_LWIN) & 0x8000) != 0 ||
                                  (GetKeyState(VK_RWIN) & 0x8000) != 0;

            if (key_down)
            {
                if (modified)
                    return;

                if (g_input.fullscreen_key_down == 0)
                {
                    finish_active_direction();
                    arknights::preserve_overlay_scale_after_resize(
                        g_input.window);
                    arknights::toggle_game_fullscreen(g_input.window);
                    g_input.fullscreen_key_down = key;
                }
                consume_message(message);
                return;
            }

            if (g_input.fullscreen_key_down == key)
                g_input.fullscreen_key_down = 0;
            consume_message(message);
            return;
        }

        if (action_shortcut_matches(key))
        {
            const bool modified = (GetKeyState(VK_CONTROL) & 0x8000) != 0 ||
                                  (GetKeyState(VK_MENU) & 0x8000) != 0 ||
                                  (GetKeyState(VK_SHIFT) & 0x8000) != 0 ||
                                  (GetKeyState(VK_LWIN) & 0x8000) != 0 ||
                                  (GetKeyState(VK_RWIN) & 0x8000) != 0;

            if (key_down)
            {
                if (modified)
                    return;

                if (g_input.action_key_down == 0)
                {
                    finish_active_direction();
                    trigger_context_action(key);
                    g_input.action_key_down = key;
                }
                consume_message(message);
                return;
            }

            if (g_input.action_key_down == key)
                g_input.action_key_down = 0;
            consume_message(message);
            return;
        }

        const direction facing = direction_from_key(key);
        if (facing == direction::none)
            return;
        if (key >= g_input.consumed_direction_keys.size())
            return;

        const bool modified = (GetKeyState(VK_CONTROL) & 0x8000) != 0 ||
                              (GetKeyState(VK_MENU) & 0x8000) != 0;

        if (key_down)
        {
            if (modified || !g_input.anchor_valid)
                return;

            g_input.consumed_direction_keys[key] = true;

            if (g_input.active_key == 0)
            {
                if (!begin_direction_drag(message, key, facing))
                    g_input.consumed_direction_keys[key] = false;
            }
            else
            {
                consume_message(message);
            }
            return;
        }

        if (!g_input.consumed_direction_keys[key])
            return;

        g_input.consumed_direction_keys[key] = false;
        consume_message(message);

        if (g_input.active_key == key)
            finish_active_direction();
    }

    LRESULT CALLBACK get_message_hook(int code, WPARAM wparam, LPARAM lparam)
    {
        if (code >= 0 && wparam == PM_REMOVE)
        {
            auto *const message = reinterpret_cast<MSG *>(lparam);
            if (message != nullptr && message->hwnd == g_input.window)
                process_game_message(*message);
        }

        return CallNextHookEx(nullptr, code, wparam, lparam);
    }


    LRESULT CALLBACK position_window_hook(int code, WPARAM wparam, LPARAM lparam)
    {
        if (code >= 0)
        {
            auto *const message = reinterpret_cast<CWPSTRUCT *>(lparam);
            if (message != nullptr && message->hwnd == g_input.window)
            {
                if (message->message == WM_MOVING && message->lParam != 0)
                {
                    const auto *const proposed =
                        reinterpret_cast<const RECT *>(message->lParam);
                    arknights::preview_game_window_move(
                        proposed->left,
                        proposed->top);
                }
                else if (message->message == WM_WINDOWPOSCHANGING &&
                         message->lParam != 0)
                {
                    const auto *const proposed =
                        reinterpret_cast<const WINDOWPOS *>(message->lParam);
                    if ((proposed->flags & SWP_NOMOVE) == 0)
                    {
                        arknights::preview_game_window_move(
                            proposed->x,
                            proposed->y);
                    }
                }
            }
        }

        return CallNextHookEx(nullptr, code, wparam, lparam);
    }

    void install_position_hook() noexcept
    {
        if (g_input.position_hook != nullptr ||
            g_input.window_thread_id == 0 ||
            GetTickCount64() < g_input.position_hook_ready_at)
        {
            return;
        }

        g_input.position_hook = SetWindowsHookExW(
            WH_CALLWNDPROC,
            position_window_hook,
            nullptr,
            g_input.window_thread_id);
        if (g_input.position_hook == nullptr)
        {
            g_input.position_hook = SetWindowsHookExW(
                WH_CALLWNDPROC,
                position_window_hook,
                g_module,
                g_input.window_thread_id);
        }
    }

    void uninstall_input_hook() noexcept
    {
        const HHOOK message_hook = g_input.message_hook;
        const HHOOK position_hook = g_input.position_hook;

        finish_active_direction();
        if (message_hook != nullptr)
            UnhookWindowsHookEx(message_hook);
        if (position_hook != nullptr)
            UnhookWindowsHookEx(position_hook);

        g_input = {};
        g_reshade_overlay_open.store(false, std::memory_order_relaxed);
    }

    void on_init_effect_runtime(reshade::api::effect_runtime *runtime)
    {
        const auto window = static_cast<HWND>(runtime->get_hwnd());
        if (window == nullptr)
            return;

        DWORD process_id = 0;
        const DWORD thread_id = GetWindowThreadProcessId(window, &process_id);
        if (thread_id == 0 || process_id != GetCurrentProcessId())
            return;

        if (g_input.window != window)
        {
            uninstall_input_hook();
            arknights::uninstall_window_enhancements();
            g_input.window = window;
        }

        g_input.window_thread_id = thread_id;
        g_input.runtime = runtime;
        static_cast<void>(arknights::initialize_unity_input());
        if (g_input.position_hook_ready_at == 0)
        {
            g_input.position_hook_ready_at =
                GetTickCount64() + k_position_hook_delay_ms;
        }
        arknights::install_window_enhancements(window, g_module);

        if (g_input.message_hook != nullptr)
            return;

        g_input.message_hook = SetWindowsHookExW(WH_GETMESSAGE, get_message_hook, nullptr, thread_id);

        if (g_input.message_hook == nullptr)
            g_input.message_hook = SetWindowsHookExW(WH_GETMESSAGE, get_message_hook, g_module, thread_id);
    }

    void on_destroy_effect_runtime(reshade::api::effect_runtime *runtime)
    {
        if (runtime == g_input.runtime)
        {
            g_input.runtime = nullptr;
            g_reshade_overlay_open.store(false, std::memory_order_relaxed);
        }
    }

    void on_reshade_present(reshade::api::effect_runtime *runtime)
    {
        if (runtime == g_input.runtime)
        {
            arknights::notify_window_presented();
            install_position_hook();
        }
    }

    bool on_reshade_open_overlay(
        reshade::api::effect_runtime *,
        bool open,
        reshade::api::input_source)
    {
        g_reshade_overlay_open.store(open, std::memory_order_relaxed);
        if (!open)
            arknights::cancel_shortcut_capture();
        if (open && g_input.window != nullptr)
            PostMessageW(g_input.window, k_cancel_drag_message, 0, 0);
        return false;
    }
}

extern "C" __declspec(dllexport) const char *NAME = "ArknightsEnhancer";
extern "C" __declspec(dllexport) const char *AUTHOR = "ItsTheSewerRat";
extern "C" __declspec(dllexport) const char *DESCRIPTION =
    "Quality-of-life features for Arknights: set deploy direction of operators "
    "with WASD, control story and gacha skips with Tab, toggle fullscreen with F12, "
    "resize the game window, and control game audio from the title bar.";

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID)
{
    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        g_module = module;
        DisableThreadLibraryCalls(module);
        if (!reshade::register_addon(module))
            return FALSE;

        arknights::load_shortcut_settings();
        reshade::register_overlay(
            "ArknightsEnhancer",
            arknights::draw_shortcut_settings);
        reshade::register_event<reshade::addon_event::init_effect_runtime>(on_init_effect_runtime);
        reshade::register_event<reshade::addon_event::destroy_effect_runtime>(on_destroy_effect_runtime);
        reshade::register_event<reshade::addon_event::reshade_present>(on_reshade_present);
        reshade::register_event<reshade::addon_event::reshade_open_overlay>(on_reshade_open_overlay);
        break;

    case DLL_PROCESS_DETACH:
        arknights::request_window_enhancements_shutdown();
        uninstall_input_hook();
        reshade::unregister_addon(module);
        break;
    }

    return TRUE;
}

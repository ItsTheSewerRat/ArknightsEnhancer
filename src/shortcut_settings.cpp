#include "shortcut_settings.hpp"

#include <imgui.h>
#include <reshade.hpp>

#include <atomic>
#include <cstddef>
#include <string>

namespace
{
    constexpr char k_config_section[] = "ArknightsEnhancer.Shortcuts";

    struct binding
    {
        const char *label = nullptr;
        const char *primary_config_key = nullptr;
        const char *alternate_config_key = nullptr;
        UINT default_key = 0;
        std::atomic_uint primary = 0;
        std::atomic_uint alternate = 0;

        binding(
            const char *binding_label,
            const char *primary_key,
            const char *alternate_key,
            UINT default_value) noexcept
            : label(binding_label),
              primary_config_key(primary_key),
              alternate_config_key(alternate_key),
              default_key(default_value),
              primary(default_value)
        {
        }
    };

    binding g_bindings[] = {
        { "Move up", "MoveUpPrimary", "MoveUpAlternate", 'W' },
        { "Move down", "MoveDownPrimary", "MoveDownAlternate", 'S' },
        { "Move left", "MoveLeftPrimary", "MoveLeftAlternate", 'A' },
        { "Move right", "MoveRightPrimary", "MoveRightAlternate", 'D' },
        { "Skip", "SkipPrimary", "SkipAlternate", VK_TAB },
        { "Gacha skip", "GachaSkipPrimary", "GachaSkipAlternate", VK_TAB },
        { "Confirm (Yes)", "ConfirmYesPrimary", "ConfirmYesAlternate", VK_TAB },
        { "Fullscreen", "FullscreenPrimary", "FullscreenAlternate", VK_F12 },
    };

    int g_capture_action = -1;
    int g_capture_slot = -1;
    std::atomic<float> g_overlay_scale = 0.0f;
    std::atomic_bool g_restore_overlay_scale = false;

    [[nodiscard]] constexpr std::size_t action_index(
        arknights::shortcut_action action) noexcept
    {
        return static_cast<std::size_t>(action);
    }

    [[nodiscard]] bool valid_key(UINT key) noexcept
    {
        return key >= VK_BACK && key <= 0xFF;
    }

    [[nodiscard]] bool modifier_key(UINT key) noexcept
    {
        return key == VK_SHIFT || key == VK_CONTROL || key == VK_MENU ||
               key == VK_LSHIFT || key == VK_RSHIFT ||
               key == VK_LCONTROL || key == VK_RCONTROL ||
               key == VK_LMENU || key == VK_RMENU ||
               key == VK_LWIN || key == VK_RWIN;
    }

    [[nodiscard]] std::string key_name(UINT key)
    {
        if (key == 0)
            return "Unbound";
        if ((key >= '0' && key <= '9') || (key >= 'A' && key <= 'Z'))
            return std::string(1, static_cast<char>(key));
        if (key >= VK_F1 && key <= VK_F24)
            return "F" + std::to_string(key - VK_F1 + 1);

        const UINT scan_code = MapVirtualKeyW(key, MAPVK_VK_TO_VSC_EX);
        LPARAM name_parameter = static_cast<LPARAM>(scan_code & 0xFF) << 16;
        if ((scan_code & 0xFF00) == 0xE000)
            name_parameter |= LPARAM { 1 } << 24;

        wchar_t wide_name[64] = {};
        const int wide_length = GetKeyNameTextW(
            static_cast<LONG>(name_parameter),
            wide_name,
            static_cast<int>(sizeof(wide_name) / sizeof(wide_name[0])));
        if (wide_length > 0)
        {
            char utf8_name[128] = {};
            const int utf8_length = WideCharToMultiByte(
                CP_UTF8,
                0,
                wide_name,
                wide_length,
                utf8_name,
                static_cast<int>(sizeof(utf8_name) - 1),
                nullptr,
                nullptr);
            if (utf8_length > 0)
                return std::string(utf8_name, static_cast<std::size_t>(utf8_length));
        }

        return "Key " + std::to_string(key);
    }

    void save_binding(binding &value, int slot, UINT key) noexcept
    {
        if (slot == 0)
        {
            value.primary.store(key, std::memory_order_relaxed);
            reshade::set_config_value(
                nullptr,
                k_config_section,
                value.primary_config_key,
                key);
        }
        else
        {
            value.alternate.store(key, std::memory_order_relaxed);
            reshade::set_config_value(
                nullptr,
                k_config_section,
                value.alternate_config_key,
                key);
        }
    }

    void draw_key_field(
        reshade::api::effect_runtime *runtime,
        std::size_t index,
        int slot)
    {
        binding &value = g_bindings[index];
        const bool capturing =
            g_capture_action == static_cast<int>(index) &&
            g_capture_slot == slot;
        const UINT current_key = slot == 0
            ? value.primary.load(std::memory_order_relaxed)
            : value.alternate.load(std::memory_order_relaxed);
        const std::string text = capturing ? "Press a key..." : key_name(current_key);

        ImGui::PushID(slot);
        if (ImGui::Button(text.c_str(), ImVec2(-1.0f, 0.0f)))
        {
            g_capture_action = static_cast<int>(index);
            g_capture_slot = slot;
        }
        if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
        {
            save_binding(value, slot, 0);
            g_capture_action = -1;
            g_capture_slot = -1;
        }
        ImGui::PopID();

        if (!capturing)
            return;

        runtime->block_input_next_frame();
        const UINT pressed_key = runtime->last_key_pressed();
        if (pressed_key == VK_ESCAPE)
        {
            g_capture_action = -1;
            g_capture_slot = -1;
        }
        else if (valid_key(pressed_key) && !modifier_key(pressed_key))
        {
            save_binding(value, slot, pressed_key);
            g_capture_action = -1;
            g_capture_slot = -1;
        }
    }
}

namespace arknights
{
    void load_shortcut_settings() noexcept
    {
        for (binding &value : g_bindings)
        {
            UINT primary = value.default_key;
            UINT alternate = 0;
            if (!reshade::get_config_value(
                    nullptr,
                    k_config_section,
                    value.primary_config_key,
                    primary) ||
                primary > 0xFF)
            {
                primary = value.default_key;
            }
            if (!reshade::get_config_value(
                    nullptr,
                    k_config_section,
                    value.alternate_config_key,
                    alternate) ||
                alternate > 0xFF)
            {
                alternate = 0;
            }

            value.primary.store(primary, std::memory_order_relaxed);
            value.alternate.store(alternate, std::memory_order_relaxed);
        }

        int font_size = 0;
        float font_scale = 0.0f;
        if (reshade::get_config_value(
                nullptr,
                "STYLE",
                "FontSize",
                font_size) &&
            font_size > 0 &&
            reshade::get_config_value(
                nullptr,
                "STYLE",
                "FontScale",
                font_scale) &&
            font_scale > 0.0f)
        {
            g_overlay_scale.store(font_scale, std::memory_order_relaxed);
        }
    }

    bool shortcut_matches(shortcut_action action, UINT key) noexcept
    {
        if (key == 0 || action >= shortcut_action::count)
            return false;

        const binding &value = g_bindings[action_index(action)];
        return key == value.primary.load(std::memory_order_relaxed) ||
               key == value.alternate.load(std::memory_order_relaxed);
    }

    void preserve_overlay_scale_after_resize(HWND game_window) noexcept
    {
        if (g_overlay_scale.load(std::memory_order_relaxed) <= 0.0f)
        {
            RECT client = {};
            if (game_window != nullptr && GetClientRect(game_window, &client))
            {
                const LONG height = client.bottom - client.top;
                const float automatic_scale =
                    height >= 2160 ? 2.0f : height >= 1440 ? 1.5f : 1.0f;
                g_overlay_scale.store(
                    automatic_scale,
                    std::memory_order_relaxed);
            }
        }

        g_restore_overlay_scale.store(true, std::memory_order_release);
    }

    void cancel_shortcut_capture() noexcept
    {
        g_capture_action = -1;
        g_capture_slot = -1;
    }

    void draw_shortcut_settings(reshade::api::effect_runtime *runtime)
    {
        if (runtime == nullptr)
            return;

        ImGuiStyle &style = ImGui::GetStyle();
        if (g_restore_overlay_scale.exchange(false, std::memory_order_acq_rel))
        {
            const float previous_scale =
                g_overlay_scale.load(std::memory_order_relaxed);
            if (previous_scale > 0.0f)
                style.FontScaleMain = previous_scale;
        }
        g_overlay_scale.store(style.FontScaleMain, std::memory_order_relaxed);

        ImGui::TextUnformatted(
            "Left-click a field to bind a key. Right-click it to unbind.");
        ImGui::Spacing();

        const float key_column_width =
            ImGui::CalcTextSize("Press a key...").x +
            ImGui::GetStyle().FramePadding.x * 2.0f;
        if (ImGui::BeginTable(
                "ShortcutBindings",
                3,
                ImGuiTableFlags_BordersInnerV |
                    ImGuiTableFlags_RowBg |
                    ImGuiTableFlags_SizingFixedFit))
        {
            ImGui::TableSetupColumn(
                "Action",
                ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn(
                "Primary",
                ImGuiTableColumnFlags_WidthFixed,
                key_column_width);
            ImGui::TableSetupColumn(
                "Alternate",
                ImGuiTableColumnFlags_WidthFixed,
                key_column_width);
            ImGui::TableHeadersRow();

            for (std::size_t index = 0;
                 index < action_index(shortcut_action::count);
                 ++index)
            {
                ImGui::PushID(static_cast<int>(index));
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::AlignTextToFramePadding();
                ImGui::TextUnformatted(g_bindings[index].label);
                ImGui::TableSetColumnIndex(1);
                draw_key_field(runtime, index, 0);
                ImGui::TableSetColumnIndex(2);
                draw_key_field(runtime, index, 1);
                ImGui::PopID();
            }

            ImGui::EndTable();
        }
    }
}

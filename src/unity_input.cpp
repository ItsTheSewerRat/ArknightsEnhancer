#include "unity_input.hpp"

#include <reshade.hpp>

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace
{
    struct Il2CppAssembly;
    struct Il2CppClass;
    struct Il2CppDomain;
    struct Il2CppException;
    struct Il2CppFieldInfo;
    struct Il2CppImage;
    struct Il2CppObject;
    struct Il2CppThread;
    struct Il2CppType;
    struct MethodInfo;

    struct il2cpp_api
    {
        Il2CppDomain *(*domain_get)() = nullptr;
        const Il2CppAssembly **(*domain_get_assemblies)(
            const Il2CppDomain *,
            std::size_t *) = nullptr;
        const Il2CppImage *(*assembly_get_image)(
            const Il2CppAssembly *) = nullptr;
        Il2CppClass *(*class_from_name)(
            const Il2CppImage *,
            const char *,
            const char *) = nullptr;
        const MethodInfo *(*class_get_method_from_name)(
            Il2CppClass *,
            const char *,
            int) = nullptr;
        const MethodInfo *(*class_get_methods)(
            Il2CppClass *,
            void **) = nullptr;
        const char *(*class_get_name)(Il2CppClass *) = nullptr;
        const char *(*class_get_namespace)(Il2CppClass *) = nullptr;
        const Il2CppFieldInfo *(*class_get_field_from_name)(
            Il2CppClass *,
            const char *) = nullptr;
        const Il2CppType *(*class_get_type)(Il2CppClass *) = nullptr;
        Il2CppClass *(*class_from_type)(const Il2CppType *) = nullptr;
        const Il2CppType *(*method_get_param)(
            const MethodInfo *,
            std::uint32_t) = nullptr;
        const char *(*method_get_name)(const MethodInfo *) = nullptr;
        std::uint32_t (*method_get_param_count)(
            const MethodInfo *) = nullptr;
        Il2CppObject *(*type_get_object)(const Il2CppType *) = nullptr;
        void (*field_get_value)(
            Il2CppObject *,
            const Il2CppFieldInfo *,
            void *) = nullptr;
        void *(*object_unbox)(Il2CppObject *) = nullptr;
        Il2CppObject *(*runtime_invoke)(
            const MethodInfo *,
            void *,
            void **,
            Il2CppException **) = nullptr;
        Il2CppThread *(*thread_current)() = nullptr;
        Il2CppThread *(*thread_attach)(Il2CppDomain *) = nullptr;
        bool loaded = false;
    };

    struct skip_binding
    {
        Il2CppClass *owner_class = nullptr;
        const MethodInfo *action = nullptr;
        const MethodInfo *availability = nullptr;
        const Il2CppFieldInfo *button_field = nullptr;
        bool button_is_component = false;
    };

    struct game_bindings
    {
        Il2CppClass *unity_object = nullptr;
        const MethodInfo *find_object_of_type = nullptr;
        const MethodInfo *component_game_object = nullptr;
        const MethodInfo *get_active_in_hierarchy = nullptr;

        Il2CppClass *direction_selector = nullptr;
        const MethodInfo *get_direction_active = nullptr;
        const MethodInfo *direction_hover = nullptr;
        const MethodInfo *direction_click = nullptr;

        skip_binding skips[6] = {};
        bool ready = false;
    };

    il2cpp_api g_api;
    game_bindings g_game;
    const char *g_stage = "not started";

    template <typename T>
    [[nodiscard]] bool load_export(
        HMODULE module,
        const char *name,
        T &function) noexcept
    {
        function = reinterpret_cast<T>(GetProcAddress(module, name));
        return function != nullptr;
    }

    int log_native_exception(unsigned long code) noexcept
    {
        char message[192] = {};
        sprintf_s(
            message,
            "ArknightsEnhancer game input stopped at '%s' "
            "(exception 0x%08lX).",
            g_stage,
            code);
        reshade::log::message(reshade::log::level::error, message);
        return EXCEPTION_EXECUTE_HANDLER;
    }

    void log_result(const char *message) noexcept
    {
        reshade::log::message(reshade::log::level::info, message);
    }

    [[nodiscard]] bool load_api() noexcept
    {
        if (g_api.loaded)
            return true;

        const HMODULE module = GetModuleHandleW(L"GameAssembly.dll");
        if (module == nullptr ||
            !load_export(module, "il2cpp_domain_get", g_api.domain_get) ||
            !load_export(
                module,
                "il2cpp_domain_get_assemblies",
                g_api.domain_get_assemblies) ||
            !load_export(
                module,
                "il2cpp_assembly_get_image",
                g_api.assembly_get_image) ||
            !load_export(
                module,
                "il2cpp_class_from_name",
                g_api.class_from_name) ||
            !load_export(
                module,
                "il2cpp_class_get_method_from_name",
                g_api.class_get_method_from_name) ||
            !load_export(
                module,
                "il2cpp_class_get_methods",
                g_api.class_get_methods) ||
            !load_export(
                module,
                "il2cpp_class_get_name",
                g_api.class_get_name) ||
            !load_export(
                module,
                "il2cpp_class_get_namespace",
                g_api.class_get_namespace) ||
            !load_export(
                module,
                "il2cpp_class_get_field_from_name",
                g_api.class_get_field_from_name) ||
            !load_export(
                module,
                "il2cpp_class_get_type",
                g_api.class_get_type) ||
            !load_export(
                module,
                "il2cpp_class_from_il2cpp_type",
                g_api.class_from_type) ||
            !load_export(
                module,
                "il2cpp_method_get_param",
                g_api.method_get_param) ||
            !load_export(
                module,
                "il2cpp_method_get_name",
                g_api.method_get_name) ||
            !load_export(
                module,
                "il2cpp_method_get_param_count",
                g_api.method_get_param_count) ||
            !load_export(
                module,
                "il2cpp_type_get_object",
                g_api.type_get_object) ||
            !load_export(
                module,
                "il2cpp_field_get_value",
                g_api.field_get_value) ||
            !load_export(
                module,
                "il2cpp_object_unbox",
                g_api.object_unbox) ||
            !load_export(
                module,
                "il2cpp_runtime_invoke",
                g_api.runtime_invoke) ||
            !load_export(
                module,
                "il2cpp_thread_current",
                g_api.thread_current) ||
            !load_export(
                module,
                "il2cpp_thread_attach",
                g_api.thread_attach))
        {
            return false;
        }

        g_api.loaded = true;
        return true;
    }

    [[nodiscard]] Il2CppClass *find_class(
        const char *name_space,
        const char *name) noexcept
    {
        Il2CppDomain *const domain = g_api.domain_get();
        if (domain == nullptr)
            return nullptr;

        std::size_t count = 0;
        const Il2CppAssembly **const assemblies =
            g_api.domain_get_assemblies(domain, &count);
        if (assemblies == nullptr)
            return nullptr;

        for (std::size_t index = 0; index < count; ++index)
        {
            const Il2CppImage *const image =
                g_api.assembly_get_image(assemblies[index]);
            if (image == nullptr)
                continue;

            if (Il2CppClass *const klass =
                    g_api.class_from_name(image, name_space, name))
            {
                return klass;
            }
        }
        return nullptr;
    }

    [[nodiscard]] bool invoke(
        const MethodInfo *method,
        void *instance,
        void **arguments,
        Il2CppObject **result = nullptr) noexcept
    {
        if (method == nullptr)
            return false;

        Il2CppException *exception = nullptr;
        Il2CppObject *const value =
            g_api.runtime_invoke(method, instance, arguments, &exception);
        if (exception != nullptr)
            return false;
        if (result != nullptr)
            *result = value;
        return true;
    }

    [[nodiscard]] bool bind_skip(
        const char *name_space,
        const char *class_name,
        const char *method_name,
        const char *button_field,
        bool button_is_component,
        skip_binding &binding) noexcept
    {
        binding.owner_class = find_class(name_space, class_name);
        if (binding.owner_class == nullptr)
            return false;

        binding.action = g_api.class_get_method_from_name(
            binding.owner_class,
            method_name,
            0);
        binding.button_field = g_api.class_get_field_from_name(
            binding.owner_class,
            button_field);
        binding.button_is_component = button_is_component;
        return binding.action != nullptr &&
               binding.button_field != nullptr;
    }

    [[nodiscard]] bool bind_skip_with_availability(
        const char *name_space,
        const char *class_name,
        const char *method_name,
        const char *availability_method,
        skip_binding &binding) noexcept
    {
        binding.owner_class = find_class(name_space, class_name);
        if (binding.owner_class == nullptr)
            return false;

        binding.action = g_api.class_get_method_from_name(
            binding.owner_class,
            method_name,
            0);
        binding.availability = g_api.class_get_method_from_name(
            binding.owner_class,
            availability_method,
            0);
        return binding.action != nullptr &&
               binding.availability != nullptr;
    }

    [[nodiscard]] const MethodInfo *find_method_with_parameter(
        Il2CppClass *klass,
        const char *method_name,
        const char *parameter_namespace,
        const char *parameter_name) noexcept
    {
        void *iterator = nullptr;
        while (const MethodInfo *const method =
                   g_api.class_get_methods(klass, &iterator))
        {
            if (std::strcmp(
                    g_api.method_get_name(method),
                    method_name) != 0 ||
                g_api.method_get_param_count(method) != 1)
            {
                continue;
            }

            Il2CppClass *const parameter_class =
                g_api.class_from_type(
                    g_api.method_get_param(method, 0));
            if (parameter_class != nullptr &&
                std::strcmp(
                    g_api.class_get_namespace(parameter_class),
                    parameter_namespace) == 0 &&
                std::strcmp(
                    g_api.class_get_name(parameter_class),
                    parameter_name) == 0)
            {
                return method;
            }
        }
        return nullptr;
    }

    [[nodiscard]] bool initialize_bindings() noexcept
    {
        if (g_game.ready)
            return true;
        if (!load_api())
            return false;

        Il2CppDomain *const domain = g_api.domain_get();
        if (domain == nullptr)
            return false;
        if (g_api.thread_current() == nullptr &&
            g_api.thread_attach(domain) == nullptr)
        {
            return false;
        }

        game_bindings value;
        value.unity_object = find_class("UnityEngine", "Object");
        Il2CppClass *const component =
            find_class("UnityEngine", "Component");
        Il2CppClass *const game_object =
            find_class("UnityEngine", "GameObject");
        value.direction_selector = find_class(
            "Torappu.Battle.UI",
            "UIDirectionSelector");
        if (value.unity_object == nullptr ||
            component == nullptr ||
            game_object == nullptr ||
            value.direction_selector == nullptr)
        {
            return false;
        }

        value.find_object_of_type = find_method_with_parameter(
            value.unity_object,
            "FindObjectOfType",
            "System",
            "Type");
        value.component_game_object =
            g_api.class_get_method_from_name(
                component,
                "get_gameObject",
                0);
        value.get_active_in_hierarchy =
            g_api.class_get_method_from_name(
                game_object,
                "get_activeInHierarchy",
                0);
        value.get_direction_active =
            g_api.class_get_method_from_name(
                value.direction_selector,
                "get_isActive",
                0);
        value.direction_hover =
            g_api.class_get_method_from_name(
                value.direction_selector,
                "OnDirectionHover",
                1);
        value.direction_click =
            g_api.class_get_method_from_name(
                value.direction_selector,
                "OnDirectionClicked",
                0);
        if (value.find_object_of_type == nullptr ||
            value.component_game_object == nullptr ||
            value.get_active_in_hierarchy == nullptr ||
            value.get_direction_active == nullptr ||
            value.direction_hover == nullptr ||
            value.direction_click == nullptr)
        {
            return false;
        }

        static_cast<void>(bind_skip(
            "Torappu.Battle.Dialog",
            "DialogPanel",
            "OnSkipClicked",
            "_skipButton",
            true,
            value.skips[0]));
        static_cast<void>(bind_skip(
            "Torappu.AVG",
            "AVGController",
            "OnSkipBtnClicked",
            "_skipBtn",
            false,
            value.skips[1]));
        static_cast<void>(bind_skip(
            "Torappu.Battle.UI.Cooperate",
            "UICooperateRestingPanel",
            "OnSkipButtonClick",
            "_skipButton",
            true,
            value.skips[2]));
        static_cast<void>(bind_skip_with_availability(
            "Torappu.Gacha",
            "GachaPhase0",
            "OnSkipAllBtnClicked",
            "get_canSkip",
            value.skips[3]));
        static_cast<void>(bind_skip_with_availability(
            "Torappu.Gacha",
            "GachaPhase1",
            "OnSkipAllBtnClicked",
            "get_canSkip",
            value.skips[4]));
        static_cast<void>(bind_skip_with_availability(
            "Torappu.Gacha",
            "GachaController",
            "OnMaskClicked",
            "get_isRunning",
            value.skips[5]));

        value.ready = true;
        g_game = value;
        return true;
    }

    [[nodiscard]] Il2CppObject *find_live_object(
        Il2CppClass *klass) noexcept
    {
        if (klass == nullptr)
            return nullptr;

        Il2CppObject *const system_type =
            g_api.type_get_object(g_api.class_get_type(klass));
        if (system_type == nullptr)
            return nullptr;

        void *arguments[] = { system_type };
        Il2CppObject *result = nullptr;
        if (!invoke(
                g_game.find_object_of_type,
                nullptr,
                arguments,
                &result))
        {
            return nullptr;
        }
        return result;
    }

    [[nodiscard]] bool unbox_bool(
        Il2CppObject *value,
        bool &result) noexcept
    {
        const auto *const raw =
            static_cast<const std::uint8_t *>(
                g_api.object_unbox(value));
        if (raw == nullptr)
            return false;
        result = *raw != 0;
        return true;
    }

    [[nodiscard]] bool is_component_active(
        Il2CppObject *component) noexcept
    {
        Il2CppObject *game_object = nullptr;
        if (!invoke(
                g_game.component_game_object,
                component,
                nullptr,
                &game_object) ||
            game_object == nullptr)
        {
            return false;
        }

        Il2CppObject *active_value = nullptr;
        bool active = false;
        return invoke(
                   g_game.get_active_in_hierarchy,
                   game_object,
                   nullptr,
                   &active_value) &&
               active_value != nullptr &&
               unbox_bool(active_value, active) &&
               active;
    }

    [[nodiscard]] bool is_button_active(
        Il2CppObject *owner,
        const skip_binding &binding) noexcept
    {
        Il2CppObject *button = nullptr;
        g_api.field_get_value(
            owner,
            binding.button_field,
            &button);
        if (button == nullptr)
            return false;

        Il2CppObject *game_object = button;
        if (binding.button_is_component &&
            (!invoke(
                 g_game.component_game_object,
                 button,
                 nullptr,
                 &game_object) ||
             game_object == nullptr))
        {
            return false;
        }

        Il2CppObject *active_value = nullptr;
        bool active = false;
        return invoke(
                   g_game.get_active_in_hierarchy,
                   game_object,
                   nullptr,
                   &active_value) &&
               active_value != nullptr &&
               unbox_bool(active_value, active) &&
               active;
    }

    [[nodiscard]] bool invoke_skip() noexcept
    {
        for (std::size_t index = 0;
             index < _countof(g_game.skips);
             ++index)
        {
            const skip_binding &binding = g_game.skips[index];
            if (binding.owner_class == nullptr)
                continue;

            Il2CppObject *const owner =
                find_live_object(binding.owner_class);
            if (owner == nullptr)
            {
                continue;
            }

            bool available = false;
            if (binding.availability != nullptr)
            {
                Il2CppObject *available_value = nullptr;
                if (!invoke(
                        binding.availability,
                        owner,
                        nullptr,
                        &available_value) ||
                    available_value == nullptr ||
                    !unbox_bool(
                        available_value,
                        available) ||
                    !available)
                {
                    continue;
                }
            }
            else if (!is_button_active(owner, binding))
            {
                continue;
            }

            g_stage = "invoke concrete Skip handler";
            if (invoke(binding.action, owner, nullptr))
            {
                char message[96] = {};
                sprintf_s(
                    message,
                    "ArknightsEnhancer Skip invoked candidate %zu.",
                    index);
                log_result(message);
                return true;
            }
        }
        return false;
    }

    [[nodiscard]] bool invoke_direction(
        POINT origin_client,
        POINT target_client) noexcept
    {
        Il2CppObject *const selector =
            find_live_object(g_game.direction_selector);
        if (selector == nullptr)
        {
            log_result(
                "ArknightsEnhancer direction selector was not found.");
            return false;
        }

        Il2CppObject *active_value = nullptr;
        bool active = false;
        if (!invoke(
                g_game.get_direction_active,
                selector,
                nullptr,
                &active_value) ||
            active_value == nullptr ||
            !unbox_bool(active_value, active) ||
            !active)
        {
            log_result(
                "ArknightsEnhancer direction selector was inactive.");
            return false;
        }

        const LONG delta_x = target_client.x - origin_client.x;
        const LONG delta_y = target_client.y - origin_client.y;
        std::int32_t direction = 0;
        if (std::labs(delta_x) >= std::labs(delta_y))
            direction = delta_x >= 0 ? 1 : 3;
        else
            direction = delta_y >= 0 ? 2 : 0;

        g_stage = "invoke UIDirectionSelector";
        void *hover_arguments[] = { &direction };
        const bool invoked =
            invoke(
                g_game.direction_hover,
                selector,
                hover_arguments) &&
            invoke(
                g_game.direction_click,
                selector,
                nullptr);
        log_result(
            invoked
                ? "ArknightsEnhancer direction handler was invoked."
                : "ArknightsEnhancer direction handler invocation failed.");
        return invoked;
    }
}

namespace arknights
{
    bool initialize_unity_input() noexcept
    {
        __try
        {
            g_stage = "initialize concrete game input";
            return initialize_bindings();
        }
        __except (log_native_exception(GetExceptionCode()))
        {
            return false;
        }
    }

    bool click_unity_ui(HWND window, POINT target_client) noexcept
    {
        static_cast<void>(window);
        static_cast<void>(target_client);
        __try
        {
            g_stage = "start concrete Skip";
            return initialize_bindings() && invoke_skip();
        }
        __except (log_native_exception(GetExceptionCode()))
        {
            return false;
        }
    }

    bool drag_unity_ui(
        HWND window,
        POINT origin_client,
        POINT target_client) noexcept
    {
        static_cast<void>(window);
        __try
        {
            g_stage = "start concrete direction";
            return initialize_bindings() &&
                   invoke_direction(
                       origin_client,
                       target_client);
        }
        __except (log_native_exception(GetExceptionCode()))
        {
            return false;
        }
    }
}

#pragma once

#include <functional>
#include <imgui.h>

using InputCallback = std::function<void()>;

namespace ItemManager {

    static void OnImGuiItemClicked(InputCallback callback)
    {
        if (ImGui::IsItemClicked() || ImGui::IsItemActive())
        {
            if (callback)
                callback();
        }
    }


    static void OnImGuiItemDeselected(InputCallback onDeselectCallback)
    {
        if (ImGui::IsItemDeactivated())
        {
            if (onDeselectCallback)
                onDeselectCallback();
        }
    }

}
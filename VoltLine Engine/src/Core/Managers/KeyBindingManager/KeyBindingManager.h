#pragma

#include "pch.h"

#include "Core/Window/Window.h"

class KeyBindingManager {
public:
    std::unordered_map<std::string, Action> keyBindings;

    void registerKeyBinding(const std::string& keyCombo, Action action) {
        keyBindings[keyCombo] = action;
    }

    bool checkKeyCombination(const std::vector<int>& pressedKeys, const std::string& keyCombo) {
        std::vector<int> expectedKeys = parseKeyCombo(keyCombo);
        if (pressedKeys == expectedKeys) {
            return true;
        }
        return false;
    }

    std::vector<int> parseKeyCombo(const std::string& keyCombo) {
        std::vector<int> keys;
        std::vector<std::string> components = splitString(keyCombo, '+');

        for (const auto& component : components) {
            if (component == "SHIFT") {
                keys.push_back(GLFW_KEY_LEFT_SHIFT);
            }
            else if (component == "CTRL") {
                keys.push_back(GLFW_KEY_LEFT_CONTROL);
            }
            else if (component == "ESC") {
                keys.push_back(GLFW_KEY_ESCAPE);
            }
            else if (component == "ALT") {
                keys.push_back(GLFW_KEY_LEFT_ALT);
            }
            else {
                std::cout << "Unrecognized key: " << component << std::endl;
            }
        }

        return keys;
    }

private:
    std::vector<std::string> splitString(const std::string& str, char delimiter) {
        std::vector<std::string> result;
        std::stringstream ss(str);
        std::string token;

        while (std::getline(ss, token, delimiter)) {
            result.push_back(token);
        }
        return result;
    }
};

KeyBindingManager keyBindingManager;

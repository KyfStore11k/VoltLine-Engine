#pragma once

#include "pch.h"

#include "nlohmann/json.hpp"
using json = nlohmann::json;

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

enum class Action {
	CloseApp
};

struct KeyCombination {
	int key;        // GLFW_KEY_*
	int modifier;   // GLFW_MOD_*
	Action action;  // Corresponding action
};

namespace Window {
	class Window
	{
	public:
		int windowW, windowH;
		std::string windowTitle;

		int Init();
		void ShowSidePanel();
		void SaveHubSettings(const json& j);
		void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
		void updateKeyBinding(Action action, const std::string& newKeyCombo);
		void ProjectButtonCallback();
		void ShowMainPanel(const std::string& screen);

		void setupCallbacks() {
			glfwSetKeyCallback(applicationWindow, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
				Window* self = static_cast<Window*>(glfwGetWindowUserPointer(window));
				if (self) {
					self->keyCallback(window, key, scancode, action, mods);
				}
				});
		}
		
	private:
		GLFWwindow* applicationWindow;
		GLuint projectIcon;
		GLuint settingsIcon;
		GLuint newProjectIcon;
		GLuint emptyProjectTemplateIcon;

		ImFont* defaultFont;
		ImFont* largeFont;
		ImFont* TitleFont;
		ImFont* ParagraphFont;
		ImFont* SubHeaderFont;

		string hubSettingsPath;
		string jsonText;
		
		string escapeCombo;

		string currentScreen = "project";
		string currentTemplate = "Empty";

		char buffer[128] = "";
	};
}
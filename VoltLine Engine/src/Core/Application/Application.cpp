#include "Core/Window/Window.h"

int main()
{
	Window::Window window;

	window.windowW = 1280;
	window.windowH = 720;
	window.windowTitle = "VoltLine Hub";

	return window.Init();
}
#pragma once

// ----- External Includes ----- //

// -- OpenGL Headers -- //

#include "glad/glad.h"
#include "GLFW/glfw3.h"

// -- IO Type Headers -- //

#include "fast_io.h"
#include "spdlog/spdlog.h"

// -- External Util Headers -- //

#include "fmt/core.h"

// ----- Internal Includes ----- //

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <stdexcept>
#include <functional>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <thread>
#include <string_view>

// ----- Using ----- //

using string = std::string;

// ----- Defines ----- //

#define tenSpace "          "
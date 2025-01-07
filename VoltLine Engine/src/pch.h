#pragma once

// ----- External Includes ----- //

// -- OpenGL Headers -- //

#include "glad/glad.h"
#include "GLFW/glfw3.h"

// -- IO Type Headers -- //

#include "fast_io.h"
#include "fast_io_device.h"

#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/basic_file_sink.h"

// -- External Util Headers -- //

#include "fmt/core.h"

// ----- Internal Includes ----- //

#include <iostream>
#include <cstring>
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
#include <filesystem>
#include <cstdlib>

// ----- Using ----- //

using string = std::string;

// ----- Defines ----- //

#define tenSpace "          "
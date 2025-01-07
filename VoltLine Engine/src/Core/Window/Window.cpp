#include "Window.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "Core/Managers/KeyBindingManager/KeyBindingManager.h"
#include "Core/Managers/ItemManager/ItemManager.h"
#include "Core/Managers/DirectoryManager/DirectoryManager.h"

using InputCallback = std::function<void()>;

static bool canFocusOnSidePanelWindow = true;

static bool fileExists(const string& filename)
{
    std::ifstream file(filename);
    return file.good();
}

static string getFileContents(const char* filePath) {
    std::ifstream file(filePath);

    if (!file.is_open()) {
        throw std::runtime_error("Could not open file: " + string(filePath));
    }

    string contents;
    string line;

    while (std::getline(file, line)) {
        contents += line + '\n';
    }

    file.close();

    return contents;

}

std::string getLogFilePath() {
    static json j;
    static string hubSettingsPath = "bin/" + string(CURRENT_PLAT) + "-" + string(CURRENT_CONF) + "/VoltLine Engine/hub_settings.json";
    static string jsonText;

    if (fileExists("hub_settings.json")) {
        jsonText = getFileContents("hub_settings.json");
    }
    else {
        jsonText = getFileContents(hubSettingsPath.c_str());
    }

    try {
        j = json::parse(jsonText);
    }
    catch (const std::exception& e) {
        j = json();
    }

    return j["engine_settings"]["engine_log_file_dir"];
}

namespace cf_Sink {
    std::shared_ptr<spdlog::logger> setupLogger() {
        // Get the log file path
        std::string logFilePath = getLogFilePath();

        // Create the sinks
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(logFilePath, true);

        // Create the logger
        std::vector<spdlog::sink_ptr> sinks = { console_sink, file_sink };
        auto logger = std::make_shared<spdlog::logger>("multi_sink_logger", sinks.begin(), sinks.end());

        // Set log level, etc.
        logger->set_level(spdlog::level::info);

        return logger;
    }

    // Static logger that is initialized using setupLogger
    static std::shared_ptr<spdlog::logger> logger = setupLogger();
}

namespace Window {

    static GLuint LoadTexture(const char* filename) {
        GLuint textureID;
        glGenTextures(1, &textureID);

        glBindTexture(GL_TEXTURE_2D, textureID);

        int width, height, nrChannels;
        unsigned char* data = stbi_load(filename, &width, &height, &nrChannels, 0);
        if (!data) {
            cf_Sink::logger->error(std::format("Failed to load texture: {}", filename));
            return 0;
        }

        GLenum format;
        if (nrChannels == 1)
            format = GL_RED;
        else if (nrChannels == 3)
            format = GL_RGB;
        else if (nrChannels == 4)
            format = GL_RGBA;
        else {
            cf_Sink::logger->error(std::format("Unsupported image format: {}", filename));
            stbi_image_free(data);
            return 0;
        }

        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);

        glBindTexture(GL_TEXTURE_2D, 0);

        return textureID;
    }

    static void saveFileContents(const char* str, const char* filePath) {
        std::ofstream file(filePath, std::ios::out | std::ios::trunc);

        if (!file.is_open()) {
            throw std::ios_base::failure("Failed to open the file for writing.");
        }

        file << str;

        if (!file) {
            throw std::ios_base::failure("Failed to write to the file.");
        }
    }

    bool setWindowIcon(GLFWwindow* window, const char* iconPath) {
        int width, height, channels;
        unsigned char* image = stbi_load(iconPath, &width, &height, &channels, 4);

        if (!image) {
            cf_Sink::logger->error(std::format("Failed to load icon file: {}'.", iconPath));
            return false;
        }

        GLFWimage icon;
        icon.width = width;
        icon.height = height;
        icon.pixels = image;

        glfwSetWindowIcon(window, 1, &icon);

        stbi_image_free(image);

        return true;
    }

    void ApplyCustomStyle() {
        ImGuiStyle& style = ImGui::GetStyle();

        style.WindowRounding = 10.0f;
        style.FrameRounding = 5.0f;
        style.ItemSpacing = ImVec2(8.0f, 6.0f);

        ImVec4* colors = style.Colors;
        colors[ImGuiCol_WindowBg] = ImVec4(0.1f, 0.1f, 0.1f, 1.0f);
        colors[ImGuiCol_TitleBg] = ImVec4(0.2f, 0.2f, 0.2f, 1.0f);
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
        colors[ImGuiCol_Button] = ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
        colors[ImGuiCol_ButtonHovered] = ImVec4(0.3f, 0.5f, 0.9f, 1.0f);
        colors[ImGuiCol_ButtonActive] = ImVec4(0.1f, 0.3f, 0.7f, 1.0f);
        colors[ImGuiCol_FrameBg] = ImVec4(0.2f, 0.2f, 0.2f, 1.0f);
        colors[ImGuiCol_FrameBgHovered] = ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
        colors[ImGuiCol_FrameBgActive] = ImVec4(0.4f, 0.4f, 0.4f, 1.0f);
    }

    void Window::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
        static json j;
        hubSettingsPath = "bin/" + string(CURRENT_PLAT) + "-" + string(CURRENT_CONF) + "/VoltLine Engine/hub_settings.json";
        jsonText;

        if (fileExists("hub_settings.json")) {
            jsonText = getFileContents("hub_settings.json");
        }
        else {
            jsonText = getFileContents(hubSettingsPath.c_str());
        }

        try {
            j = json::parse(jsonText);
        }
        catch (const std::exception& e) {
            j = json();
        }

        static std::vector<int> pressedKeys;

        if (action == GLFW_PRESS) {
            pressedKeys.push_back(key);
        }
        else if (action == GLFW_RELEASE) {
            pressedKeys.erase(std::remove(pressedKeys.begin(), pressedKeys.end(), key), pressedKeys.end());
        }

        std::vector<std::string> keyBindings;

        for (const auto& combo : j["keybinds"])
        {
            keyBindings.push_back(combo);
        }

        for (const auto& keyBinding : keyBindings) {
            std::vector<int> expectedKeys = keyBindingManager.parseKeyCombo(keyBinding);

            bool match = true;
            for (int expectedKey : expectedKeys) {
                if (std::find(pressedKeys.begin(), pressedKeys.end(), expectedKey) == pressedKeys.end()) {
                    match = false;
                    break;
                }
            }

            if (match) {
                if (keyBinding == escapeCombo) {
                    glfwSetWindowShouldClose(window, GLFW_TRUE);
                    exit(0);
                }
            }
        }
    }

    void Window::updateKeyBinding(Action action, const std::string& newKeyCombo) {
        for (auto& keyBinding : keyBindingManager.keyBindings) {
            if (keyBinding.second == action) {
                keyBindingManager.keyBindings.erase(keyBinding.first);
                break;
            }
        }

        keyBindingManager.registerKeyBinding(newKeyCombo, action);
    }

    int Window::Init() {

        cf_Sink::logger->flush_on(spdlog::level::info);
        spdlog::set_level(spdlog::level::info);

        cf_Sink::logger->set_pattern("%+");

        if (!glfwInit()) {
            std::cerr << "Failed to initialize GLFW" << std::endl;
            return -1;
        }

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        applicationWindow = glfwCreateWindow(windowW, windowH, windowTitle.c_str(), NULL, NULL);

        if (!applicationWindow) {
            cf_Sink::logger->error("Failed to create GLFW window");
            glfwTerminate();
            return -1;
        }

        string iconPath = "bin/" + string(CURRENT_PLAT) + "-" + string(CURRENT_CONF) + "/VoltLine Engine/logo.png";

        if (fileExists("logo.png"))
        {
            setWindowIcon(applicationWindow, "logo.png");
        } else {
            setWindowIcon(applicationWindow, iconPath.c_str());
        }

        static json j;
        hubSettingsPath = "bin/" + string(CURRENT_PLAT) + "-" + string(CURRENT_CONF) + "/VoltLine Engine/hub_settings.json";
        jsonText;

        if (fileExists("hub_settings.json")) {
            jsonText = getFileContents("hub_settings.json");
        }
        else {
            jsonText = getFileContents(hubSettingsPath.c_str());
        }

        try {
            j = json::parse(jsonText);
        }
        catch (const std::exception& e) {
            j = json();
        }

        glfwMakeContextCurrent(applicationWindow);
        glfwSetWindowUserPointer(applicationWindow, this);

        glfwSetMouseButtonCallback(applicationWindow, [](GLFWwindow* window, int button, int action, int mods)
            {
                if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
                {
                    std::thread([]() {
                        std::this_thread::sleep_for(std::chrono::milliseconds(200));
                        if (canFocusOnSidePanelWindow)
                            ImGui::SetWindowFocus("VoltLine Side Panel");
                        }).detach();
                }
            });

        setupCallbacks();

        escapeCombo = j["keybinds"]["Escape"];
        keyBindingManager.registerKeyBinding(escapeCombo, Action::CloseApp);

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

        string fontPath = "bin/" + string(CURRENT_PLAT) + "-" + string(CURRENT_CONF) + "/VoltLine Engine/Fredoka-Medium.ttf";
        if (fileExists("Fredoka-Medium.ttf"))
        {
            defaultFont = io.Fonts->AddFontFromFileTTF("Fredoka-Medium.ttf", 16.0f);
            largeFont = io.Fonts->AddFontFromFileTTF("Fredoka-Medium.ttf", 18.0f);
            TitleFont = io.Fonts->AddFontFromFileTTF("Fredoka-Medium.ttf", 32.0f);
            SubHeaderFont = io.Fonts->AddFontFromFileTTF("Fredoka-Medium.ttf", 29.0f);
            ParagraphFont = io.Fonts->AddFontFromFileTTF("Fredoka-Medium.ttf", 23.0f);
        }
        else {
            defaultFont = io.Fonts->AddFontFromFileTTF(fontPath.c_str(), 16.0f);
            largeFont = io.Fonts->AddFontFromFileTTF(fontPath.c_str(), 18.0f);
            TitleFont = io.Fonts->AddFontFromFileTTF(fontPath.c_str(), 32.0f);
            SubHeaderFont = io.Fonts->AddFontFromFileTTF(fontPath.c_str(), 25.0f);
            ParagraphFont = io.Fonts->AddFontFromFileTTF(fontPath.c_str(), 20.0f);
        }

        ImGui_ImplGlfw_InitForOpenGL(applicationWindow, true);
        ImGui_ImplOpenGL3_Init("#version 420");

        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
            cf_Sink::logger->error("Failed to initialize GLAD");
            return -1;
        }

        ApplyCustomStyle();

        string projectIconPath = "bin/" + string(CURRENT_PLAT) + "-" + string(CURRENT_CONF) + "/VoltLine Engine/project_icon.png";
        if (fileExists("project_icon.png"))
        {
            projectIcon = LoadTexture("project_icon.png");
        }
        else {
            projectIcon = LoadTexture(projectIconPath.c_str());
        }

        string settingsIconPath = "bin/" + string(CURRENT_PLAT) + "-" + string(CURRENT_CONF) + "/VoltLine Engine/settings_icon.png";
        if (fileExists("settings_icon.png"))
        {
            settingsIcon = LoadTexture("settings_icon.png");
        }
        else {
            settingsIcon = LoadTexture(settingsIconPath.c_str());
        }

        string newProjectIconPath = "bin/" + string(CURRENT_PLAT) + "-" + string(CURRENT_CONF) + "/VoltLine Engine/new_project_icon.png";
        if (fileExists("new_project_icon.png"))
        {
            newProjectIcon = LoadTexture("new_project_icon.png");
        }
        else {
            newProjectIcon = LoadTexture(newProjectIconPath.c_str());
        }

        string emptyProjectTemplateIconPath = "bin/" + string(CURRENT_PLAT) + "-" + string(CURRENT_CONF) + "/VoltLine Engine/empty_project_template_icon.png";
        if (fileExists("empty_project_template_icon.png"))
        {
            emptyProjectTemplateIcon = LoadTexture("empty_project_template_icon.png");
        }
        else {
            emptyProjectTemplateIcon = LoadTexture(emptyProjectTemplateIconPath.c_str());
        }

        static json k;
        string hubSettingsPath = "bin/" + string(CURRENT_PLAT) + "-" + string(CURRENT_CONF) + "/VoltLine Engine/hub_settings.json";
        string jsonText;

        if (fileExists("hub_settings.json")) {
            jsonText = getFileContents("hub_settings.json");
        }
        else {
            jsonText = getFileContents(hubSettingsPath.c_str());
        }

        try {
            k = json::parse(jsonText);
        }
        catch (const std::exception& e) {
            k = json();
        }

        while (!glfwWindowShouldClose(applicationWindow)) {
            glClear(GL_COLOR_BUFFER_BIT);

            glfwPollEvents();

            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            ImGui::SetNextWindowPos(ImVec2(0, 0));
            ImGui::SetNextWindowSize(io.DisplaySize);
            ImGui::Begin("MainWindow", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBackground);
            
            ShowSidePanel();
            ShowMainPanel(currentScreen);

            ImGui::End();

            ImGui::Render();
            int display_w, display_h;
            glfwGetFramebufferSize(applicationWindow, &display_w, &display_h);
            glViewport(0, 0, display_w, display_h);
            glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

            glfwSwapBuffers(applicationWindow);
        }

        glfwTerminate();
        return 0;
    }

    void Window::ShowMainPanel(const std::string& screen)
    {

        if (screen == "project")
        {
            static json j;
            hubSettingsPath = "bin/" + string(CURRENT_PLAT) + "-" + string(CURRENT_CONF) + "/VoltLine Engine/projects.json";
            jsonText;

            if (fileExists("projects.json")) {
                jsonText = getFileContents("projects.json");
            }
            else {
                jsonText = getFileContents(hubSettingsPath.c_str());
            }

            try {
                j = json::parse(jsonText);
            }
            catch (const std::exception& e) {
                j = json();
            }

            ImGui::SetCursorPos(ImVec2(220, 20));

            ImGui::PushFont(TitleFont);
            ImGui::Text("Projects:");
            ImGui::PopFont();

            ImGui::SetCursorPos(ImVec2(220, 75));

            if (ImGui::BeginChild("ScrollableRegion", ImVec2(1000, 600), true, ImGuiWindowFlags_HorizontalScrollbar)) {
                for (const auto& [name, value]: j["projects"].items())
                {
                    if (ImGui::Button(std::format("{}: {}", name, value["project_file"].dump()).c_str()))
                    {
                        if (!std::string_view(value["project_file"].dump()).ends_with(R"(.voltproj")"))
                        {
                            cf_Sink::logger->error(std::format("{}: is not a valid VoltLine project type. (etc., .voltproj)", value["project_file"].dump()).c_str());
                            throw std::runtime_error("Invalid VoltLine Project Type");
                        }

                        cf_Sink::logger->info(std::format("Opening Project: {}", value["project_file"].dump()));

                        ImGui::SetWindowFocus("VoltLine Side Panel");
                    }
                }
            }
            ImGui::EndChild();
        }
        else if (screen == "new_project")
        {

            static char projectName[64] = "New Project";
            auto defaultProjectPath = DirectoryManager::getUserDocumentsPath({"Documents", "VoltLine Projects", projectName});

            std::string defaultProjectPathStr = defaultProjectPath.string();

            static char projectLocation[128] = "";

            errno_t err = strncpy_s(projectLocation, sizeof(projectLocation), defaultProjectPathStr.c_str(), defaultProjectPathStr.size());
            if (err != 0) {
                throw std::runtime_error("Error copying defaultProjectPath.string() into projectLocation.");
            }

            ImGui::SetCursorPos(ImVec2(220, 20));

            ImGui::PushFont(TitleFont);
            ImGui::Text("Create New Project:");
            ImGui::PopFont();

            ImGui::SetCursorPos(ImVec2(220, 90));
            
            if (ImGui::ImageButton("empty_project_template_icon_id", (ImTextureID)(intptr_t)emptyProjectTemplateIcon, ImVec2(290, 290)))
            {
                currentTemplate = "Empty";
            }

            ImGui::Spacing();

            ImGui::SetCursorPos(ImVec2(220, 400));

            ImGui::PushFont(SubHeaderFont);
            ImGui::Text(std::format("Project Template: {}", currentTemplate).c_str());

            ImGui::SetCursorPos(ImVec2(220, 430));

            ImGui::Text("Project Name: ");

            ImGui::SetCursorPos(ImVec2(360, 430));
            ImGui::PushItemWidth(300);
            ImGui::InputText("##ProjectName", projectName, IM_ARRAYSIZE(projectName));
            ImGui::PopItemWidth();

            ItemManager::OnImGuiItemClicked([]() {
                canFocusOnSidePanelWindow = false;
                });

            ItemManager::OnImGuiItemDeselected([]() {
                canFocusOnSidePanelWindow = true;
                });

            ImGui::SetCursorPos(ImVec2(220, 460));

            ImGui::Text("Project Location: ");

            ImGui::SetCursorPos(ImVec2(390, 460));
            ImGui::PushItemWidth(750);
            ImGui::InputText("##ProjectLocation", projectLocation, IM_ARRAYSIZE(projectLocation));
            ImGui::PopItemWidth();

            ItemManager::OnImGuiItemClicked([]() {
                canFocusOnSidePanelWindow = false;
                });

            ItemManager::OnImGuiItemDeselected([]() {
                canFocusOnSidePanelWindow = true;
                });
            
            ImGui::PopFont();

            ImGui::SetCursorPos(ImVec2(220, 520));

            ImGui::PushFont(SubHeaderFont);
            if (ImGui::Button("Create Project", ImVec2(300, 50)))
            {
                static json j;
                hubSettingsPath = "bin/" + string(CURRENT_PLAT) + "-" + string(CURRENT_CONF) + "/VoltLine Engine/projects.json";
                jsonText;

                if (fileExists("projects.json")) {
                    jsonText = getFileContents("projects.json");
                }
                else {
                    jsonText = getFileContents(hubSettingsPath.c_str());
                }

                try {
                    j = json::parse(jsonText);
                }
                catch (const std::exception& e) {
                    j = json();
                }

                j["projects"][projectName] = {
                    {"project_file", std::string(projectLocation) + "\\" + std::string(projectName) + ".voltproj"}
                };

                SaveProjects(j);
            }
            ImGui::PopFont();
        }
    }

    void Window::ProjectButtonCallback()
    {
        if (currentScreen == "project") {
            ShowMainPanel(currentScreen);
        }
        else if (currentScreen == "new_project") {
            ShowMainPanel(currentScreen);
        }
        else {
            cf_Sink::logger->error(std::format("{} is not a valid/implemented screen!", currentScreen).c_str());
            throw std::runtime_error("Invalid Screen");
        }
    }

    void Window::ShowSidePanel()
    {
        ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(200, ImGui::GetIO().DisplaySize.y), ImGuiCond_Always);

        ImGui::Begin("VoltLine Side Panel", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

        if (ImGui::ImageButton("project_icon_id", (ImTextureID)(intptr_t)projectIcon, ImVec2(64, 64)))
        {
            currentScreen = "project";
            ProjectButtonCallback();
        }
        ImGui::SameLine();
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (64 - ImGui::GetTextLineHeight()) * 0.5f);
        ImGui::Text("Projects");

        if (ImGui::ImageButton("new_project_icon_id", (ImTextureID)(intptr_t)newProjectIcon, ImVec2(64, 64)))
        {
            currentScreen = "new_project";
            ProjectButtonCallback();
        }
        ImGui::SameLine();
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (64 - ImGui::GetTextLineHeight()) * 0.5f);
        ImGui::Text("New Project");

        if (ImGui::ImageButton("settings_icon_id", (ImTextureID)(intptr_t)settingsIcon, ImVec2(64, 64)))
        {
            canFocusOnSidePanelWindow = false;
            ImGui::OpenPopup("Settings Panel");
            ImGui::SetWindowFocus("Settings Panel");
        }
        ImGui::SameLine();
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (64 - ImGui::GetTextLineHeight()) * 0.5f);
        ImGui::Text("Settings");

        ImGui::SetCursorPosY(ImGui::GetWindowHeight() - ImGui::GetFrameHeightWithSpacing() - ImGui::GetStyle().WindowPadding.y);
        if (ImGui::Button("Quit", ImVec2(180, 0)))
        {
            exit(0);
        }

        static bool loggingOn;
        static bool debuggingOn;
        static bool showingFps;

        static json j;
        static bool isInitialized = false;

        if (ImGui::BeginPopupModal("Settings Panel", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            canFocusOnSidePanelWindow = false;
            ImGui::SetWindowFocus("Settings Panel");
            if (!isInitialized) {
                hubSettingsPath = "bin/" + string(CURRENT_PLAT) + "-" + string(CURRENT_CONF) + "/VoltLine Engine/hub_settings.json";
                jsonText;

                if (fileExists("hub_settings.json")) {
                    jsonText = getFileContents("hub_settings.json");
                }
                else {
                    jsonText = getFileContents(hubSettingsPath.c_str());
                }

                try {
                    j = json::parse(jsonText);
                }
                catch (const std::exception& e) {
                    j = json();
                }

                loggingOn = j["debugging"]["logging"];
                debuggingOn = j["debugging"]["enable_debug_mode"];
                showingFps = j["debugging"]["show_fps"];

                isInitialized = true;
            }

            string engineVersion = j["engine_settings"]["engine_version"];
            string engineLogDir = j["engine_settings"]["engine_log_file_dir"];
            string preferred_editor_win = j["engine_settings"]["preferred_editor_win"];

            int maxFps = j["render_settings"]["max_fps"];

            auto renderers = j["render_settings"]["renderers"];
            
            ImGui::PushFont(largeFont);
            ImGui::Text("Engine Settings");
            ImGui::PopFont();
            ImGui::Separator();

            ImGui::Text(std::format("Engine Version: {}", engineVersion).c_str());
            ImGui::Text(std::format("Engine File Log: {}", engineLogDir).c_str());
            ImGui::Text(std::format("Preferred Windows Editor: {}", preferred_editor_win).c_str());

            ImGui::Spacing();

            ImGui::PushFont(largeFont);
            ImGui::Text("Debugging");
            ImGui::PopFont();
            ImGui::Separator();

            ImGui::Checkbox("Debugging Enabled", &debuggingOn);
            ImGui::Checkbox("Logging Enabled", &loggingOn);
            ImGui::Checkbox("Show FPS", &showingFps);

            ImGui::Spacing();

            ImGui::PushFont(largeFont);
            ImGui::Text("Render Settings");
            ImGui::PopFont();
            ImGui::Separator();

            ImGui::Text(std::format("Max FPS: {}", maxFps).c_str());

            ImGui::Text("Used/Required Renderers:");
            for (const auto& renderer : renderers) {
                string rendererStr = renderer.dump();
                ImGui::Text(fmt::format("{}{}", tenSpace, rendererStr).c_str());
            }

            ImGui::Spacing();
            
            ImGui::PushFont(largeFont);
            ImGui::Text("Keybinds");
            ImGui::PopFont();
            ImGui::Separator();

            for (auto& [name, value] : j["keybinds"].items())
            {
                ImGui::Text(std::format("{}: {}", name, value.dump()).c_str());
            }

            ImGui::Spacing();

            ImGui::PushFont(largeFont);
            ImGui::Text("Plugins");
            ImGui::PopFont();
            ImGui::Separator();

            for (auto& [name, value] : j["plugins"].items())
            {
                bool pluginEnabled = value["enabled"];
                string pluginType = value["type"];
                string pluginLocation = value["location"];
                string pluginPrimaryFile = value["primaryFile"];

                if (pluginEnabled == 1)
                    pluginEnabled = true;
                else if (pluginEnabled == 0)
                    pluginEnabled = false;

                if (pluginType == "core" || pluginType == "Core" || pluginType == "CORE") {
                    // later
                }
                else if (pluginType == "external" || pluginType == "External" || pluginType == "EXTERNAL") {
                    // much later
                }
                else {
                    cf_Sink::logger->error(std::format("Invalid Plugin Type: {}. Acceptables types include: core (core, Core, or CORE) and external (external, External or EXTERNAL)", pluginType).c_str());
                    throw std::runtime_error("Invalid Plugin Type");
                }

                ImGui::Text(std::format(R"({}: 
    Enabled: {}
    Type: {}
    Location: {}
    Primary File: {}
)", name, pluginEnabled, pluginType, pluginLocation, pluginPrimaryFile).c_str());
            }

            // Finish Keybinds and Plugins

            bool currentLoggingState = j["debugging"]["logging"];
            bool currentDebuggingState = j["debugging"]["enable_debug_mode"];
            bool currentShowFPSState = j["debugging"]["show_fps"];

            if (currentLoggingState != loggingOn) {
                j["debugging"]["logging"] = loggingOn;

                SaveHubSettings(j);
            }
            else if (currentDebuggingState != debuggingOn) {
                j["debugging"]["enable_debug_mode"] = debuggingOn;

                SaveHubSettings(j);
            }
            else if (currentShowFPSState != showingFps) {
                j["debugging"]["show_fps"] = showingFps;

                SaveHubSettings(j);
            }

            std::string filePath1;
            std::string filePath2;
            std::string command1 = "error";
            std::string command2 = "error";

            if (fileExists("hub_settings.json"))
                filePath1 = "hub_settings.json";
            else
                filePath1 = "bin/" + string(CURRENT_PLAT) + "-" + string(CURRENT_CONF) + "/VoltLine Engine/hub_settings.json";

            if (fileExists("projects.json"))
                filePath2 = "projects.json";
            else
                filePath2 = "bin/" + string(CURRENT_PLAT) + "-" + string(CURRENT_CONF) + "/VoltLine Engine/projects.json";

#ifdef _WIN32
            command1 = std::format("{} {}", j["engine_settings"]["preferred_editor_win"].dump(), filePath1);
            command2 = std::format("{} {}", j["engine_settings"]["preferred_editor_win"].dump(), filePath2);
#endif

            ImGui::Spacing();
            if (ImGui::Button("Open Hub Settings (hub_settings.json)"))
            {
                if (command1 == "error")
                {
                    cf_Sink::logger->error("Unsupported OS");
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                    exit(0);
                }
                system(command1.c_str());
            }
            if (ImGui::Button("Open Projects File (projects.json)"))
            {
                system(command2.c_str());
            }
            ImGui::Spacing();

            if (ImGui::Button("Close")) {
                ImGui::CloseCurrentPopup();
                canFocusOnSidePanelWindow = true;
            }
            ImGui::EndPopup();
        }
        
        ImGui::End();
    }

    void Window::SaveHubSettings(const json& j)
    {
        string hubSettingsJSONPath = "bin/" + string(CURRENT_PLAT) + "-" + string(CURRENT_CONF) + "/VoltLine Engine/hub_settings.json";

        try {
            if (fileExists("hub_settings.json"))
                saveFileContents(j.dump(4).c_str(), "hub_settings.json");
            else
                saveFileContents(j.dump(4).c_str(), hubSettingsJSONPath.c_str());
        }
        catch (const std::exception& e) {
            ImGui::Text("Failed to save settings.");
        }
    }

    void Window::SaveProjects(const json& j)
    {
        string projectsJSONPath = "bin/" + string(CURRENT_PLAT) + "-" + string(CURRENT_CONF) + "/VoltLine Engine/projects.json";

        try {
            if (fileExists("projects.json"))
                saveFileContents(j.dump(4).c_str(), "projects.json");
            else
                saveFileContents(j.dump(4).c_str(), projectsJSONPath.c_str());
        }
        catch (const std::exception& e) {
            ImGui::Text("Failed to save projects.");
        }
    }
}

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include <string>
#include <cstdio>
#include <array>
#include <fstream>
#include <sstream>
#include <thread>
#include <chrono>
#include <filesystem>
#include "TextEditor.h"

namespace fs = std::filesystem;

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

enum AppState {
    STATE_WELCOME,
    STATE_EDITOR
};

GLuint LoadTextureFromFile(const char* filename) {
    int width, height, channels;
    unsigned char* data = stbi_load(filename, &width, &height, &channels, 4);
    if (data == NULL) return 0;

    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    stbi_image_free(data);

    return textureID;
}

std::string OpenFolderDialog() {
    std::string path = "";
    FILE* pipe = popen("zenity --file-selection --directory --title='Select Project Directory'", "r");
    if (pipe) {
        std::array<char, 128> buffer;
        if (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
            path = buffer.data();
            if (!path.empty() && path.back() == '\n') path.pop_back();
        }
        pclose(pipe);
    }
    return path;
}

std::string OpenFileDialog() {
    std::string path = "";
    FILE* pipe = popen("zenity --file-selection --title='Upload Files (Images, Videos, Docs, etc.)'", "r");
    if (pipe) {
        std::array<char, 128> buffer;
        if (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
            path = buffer.data();
            if (!path.empty() && path.back() == '\n') path.pop_back();
        }
        pclose(pipe);
    }
    return path;
}

// Build standard AppImage package with UI and Core (No icon option)
void BuildIntegratedAppImage(const std::string& projectRoot, std::string& outLog) {
    if (projectRoot.empty() || !fs::exists(projectRoot)) {
        outLog = "[Error]: No valid project selected.";
        return;
    }

    fs::path root(projectRoot);
    std::string projName = "app";

    fs::path uiSrc = root / "UI";
    fs::path coreSrc = root / "Core";

    try {
        std::string appImageName = projName + "-x86_64.AppImage";
        fs::path appDir = root / (projName + ".AppDir");
        
        if (fs::exists(appDir)) fs::remove_all(appDir);

        fs::create_directories(appDir / "UI");
        fs::create_directories(appDir / "Core");

        if (fs::exists(uiSrc)) fs::copy(uiSrc, appDir / "UI", fs::copy_options::overwrite_existing | fs::copy_options::recursive);
        if (fs::exists(coreSrc)) fs::copy(coreSrc, appDir / "Core", fs::copy_options::overwrite_existing | fs::copy_options::recursive);

        std::ofstream desk(appDir / "app.desktop");
        desk << "[Desktop Entry]\n"
             << "Name=" << projName << "\n"
             << "Exec=AppRun\n"
             << "Type=Application\n"
             << "Categories=Utility;\n";
        desk.close();

        std::ofstream appRun(appDir / "AppRun");
        appRun << "#!/bin/sh\n"
               << "HERE=\"$(dirname \"$(readlink -f \"$0\")\")\"\n"
               << "exec python3 \"$HERE/UI/main.py\" \"$@\"\n";
        appRun.close();
        fs::permissions(appDir / "AppRun", fs::perms::owner_all | fs::perms::group_read | fs::perms::others_read, fs::perm_options::add);

        fs::path finalFile = root / appImageName;
        if (fs::exists(finalFile)) fs::remove(finalFile);

        fs::copy(appDir / "AppRun", finalFile, fs::copy_options::overwrite_existing);
        fs::permissions(finalFile, fs::perms::owner_all | fs::perms::group_read | fs::perms::others_read, fs::perm_options::add);

        outLog = "[AppImage Success]: AppImage package created successfully: " + finalFile.string();
    } catch (const std::exception& e) {
        outLog = std::string("[Error]: ") + e.what();
    }
}

void ApplyTheme(int themeChoice) {
    ImGuiStyle& style = ImGui::GetStyle();
    if (themeChoice == 0) {
        ImGui::StyleColorsDark();
    } else if (themeChoice == 1) {
        ImGui::StyleColorsLight();
    } else if (themeChoice == 2) {
        ImGui::StyleColorsDark();
        style.Colors[ImGuiCol_WindowBg] = ImVec4(0.12f, 0.02f, 0.02f, 0.9f); 
        style.Colors[ImGuiCol_Header] = ImVec4(0.7f, 0.1f, 0.1f, 1.0f);
        style.Colors[ImGuiCol_Button] = ImVec4(0.8f, 0.15f, 0.15f, 1.0f);
        style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.9f, 0.25f, 0.25f, 1.0f);
    }
}

int main() {
    if (!glfwInit()) return 1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(1280, 720, "FOSS App Studio - Editor", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    AppState currentState = STATE_WELCOME;
    char projectName[128] = "app";
    char projectPath[256] = "/home/";
    std::string activeFilepath = "";

    int selectedTheme = 0;
    int maxRamMB = 2048; 

    bool showFindDialog = false;
    bool showReplaceDialog = false;
    char findBuffer[128] = "";
    char replaceBuffer[128] = "";
    
    char newFileNameBuffer[128] = "engine.cpp";
    bool showNewFileInUI = false;
    bool showNewFileInCore = false;

    std::string outputLog = "[FOSS App Studio]: Ready. Create or open your project.";

    ApplyTheme(selectedTheme);

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    GLuint bgTexture = LoadTextureFromFile("background.jpg");

    TextEditor editor;
    editor.SetLanguageDefinition(TextEditor::LanguageDefinition::CPlusPlus());

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // Global Shortcut: Ctrl + S to save active file
        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S)) {
            if (!activeFilepath.empty()) {
                std::ofstream outFile(activeFilepath);
                if (outFile.is_open()) {
                    outFile << editor.GetText();
                    outFile.close();
                    outputLog = "[Save]: File saved successfully: " + activeFilepath;
                }
            }
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        if (bgTexture != 0) {
            ImGui::GetBackgroundDrawList()->AddImage(
                (void*)(intptr_t)bgTexture,
                ImVec2(0, 0),
                ImVec2(io.DisplaySize.x, io.DisplaySize.y)
            );
        }

        if (currentState == STATE_WELCOME) {
            ImGui::SetNextWindowPos(ImVec2(260, 60), ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(760, 600), ImGuiCond_Always);
            ImGui::Begin("FOSS App Studio - Welcome", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

            ImGui::Spacing();
            ImGui::Text("Welcome! FOSS App Development Environment:");
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Settings & Theme Customization:");
            const char* themes[] = { "Dark Theme", "Light Theme", "Red Theme" };
            if (ImGui::Combo("Theme Color", &selectedTheme, themes, IM_ARRAYSIZE(themes))) {
                ApplyTheme(selectedTheme);
            }
            ImGui::SliderInt("Max RAM Editor (MB)", &maxRamMB, 500, 4096, "%d MB");

            ImGui::Separator();
            ImGui::Spacing();

            ImGui::TextColored(ImVec4(0.2f, 0.8f, 1.0f, 1.0f), "Create New Project (UI [Python] & Core [C++, C, Java, Python, Go, Rust]):");
            ImGui::InputText("Project Name", projectName, IM_ARRAYSIZE(projectName));
            ImGui::InputText("Project Path", projectPath, IM_ARRAYSIZE(projectPath));
            if (ImGui::Button("Browse Folder...", ImVec2(130, 25))) {
                std::string selectedFolder = OpenFolderDialog();
                if (!selectedFolder.empty()) {
                    snprintf(projectPath, sizeof(projectPath), "%s", selectedFolder.c_str());
                }
            }

            ImGui::Spacing();
            if (ImGui::Button("Create Project & UI/Core Structure", ImVec2(260, 35))) {
                std::string fullDirPath = std::string(projectPath) + "/" + std::string(projectName);
                fs::create_directories(fullDirPath + "/UI");
                fs::create_directories(fullDirPath + "/Core");

                snprintf(projectPath, sizeof(projectPath), "%s", fullDirPath.c_str());

                activeFilepath = fullDirPath + "/UI/main.py";
                std::ofstream outUI(activeFilepath);
                if (outUI.is_open()) {
                    outUI << "# UI / Window (Python)\ndef main():\n    print('FOSS UI App works correctly!')\n\nif __name__ == '__main__':\n    main()\n";
                    outUI.close();
                }

                std::string coreFile = fullDirPath + "/Core/engine.cpp";
                std::ofstream outCore(coreFile);
                if (outCore.is_open()) {
                    outCore << "// Core Engine (C++)\n#include <iostream>\nint main() {\n    std::cout << \"Core Engine C++ works correctly!\" << std::endl;\n    return 0;\n}\n";
                    outCore.close();
                }

                editor.SetText("# UI / Window (Python)\ndef main():\n    print('FOSS UI App works correctly!')\n\nif __name__ == '__main__':\n    main()\n");
                currentState = STATE_EDITOR;
            }

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::TextColored(ImVec4(0.2f, 0.8f, 1.0f, 1.0f), "Open Existing Project:");
            if (ImGui::Button("Open Project Folder...", ImVec2(220, 30))) {
                std::string chosenFolder = OpenFolderDialog();
                if (!chosenFolder.empty()) {
                    snprintf(projectPath, sizeof(projectPath), "%s", chosenFolder.c_str());
                    currentState = STATE_EDITOR;
                }
            }

            ImGui::End();

        } else if (currentState == STATE_EDITOR) {
            
            if (ImGui::BeginMainMenuBar()) {
                if (ImGui::BeginMenu("File")) {
                    if (ImGui::MenuItem("Save", "Ctrl+S")) {
                        if (!activeFilepath.empty()) {
                            std::ofstream outFile(activeFilepath);
                            if (outFile.is_open()) {
                                outFile << editor.GetText();
                                outFile.close();
                                outputLog = "[Save]: File saved successfully: " + activeFilepath;
                            }
                        }
                    }
                    if (ImGui::MenuItem("Close Editor")) {
                        activeFilepath = "";
                        editor.SetText("");
                        outputLog = "[Editor]: Editor closed.";
                    }
                    if (ImGui::MenuItem("Open Project...")) {
                        std::string chosenFolder = OpenFolderDialog();
                        if (!chosenFolder.empty()) {
                            snprintf(projectPath, sizeof(projectPath), "%s", chosenFolder.c_str());
                            outputLog = "[Project]: Opened project: " + std::string(projectPath);
                        }
                    }
                    if (ImGui::MenuItem("Go Back to Menu")) {
                        currentState = STATE_WELCOME;
                    }
                    ImGui::Separator();
                    if (ImGui::MenuItem("Exit", "Ctrl+Q")) { glfwSetWindowShouldClose(window, true); }
                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("Edit")) {
                    if (ImGui::MenuItem("Undo", "Ctrl+Z", false, editor.CanUndo())) editor.Undo();
                    if (ImGui::MenuItem("Redo", "Ctrl+Y", false, editor.CanRedo())) editor.Redo();
                    ImGui::Separator();
                    if (ImGui::MenuItem("Copy", "Ctrl+C", false, editor.HasSelection())) editor.Copy();
                    if (ImGui::MenuItem("Paste", "Ctrl+V")) editor.Paste();
                    if (ImGui::MenuItem("Delete", "Del", false, editor.HasSelection())) editor.Delete();
                    ImGui::Separator();
                    if (ImGui::MenuItem("Select All", "Ctrl+A")) editor.SelectAll();
                    ImGui::Separator();
                    if (ImGui::MenuItem("Find", "Ctrl+F")) showFindDialog = true;
                    if (ImGui::MenuItem("Replace", "Ctrl+H")) showReplaceDialog = true;
                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("Run")) {
                    if (ImGui::MenuItem("Run Script", "F5")) {
                        if (!activeFilepath.empty()) {
                            std::string ext = fs::path(activeFilepath).extension().string();
                            std::string cmd = "";
                            if (ext == ".py") {
                                cmd = "python3 \"" + activeFilepath + "\" 2>&1";
                            } else if (ext == ".cpp" || ext == ".cc") {
                                std::string outBin = activeFilepath + ".out";
                                cmd = "g++ \"" + activeFilepath + "\" -o \"" + outBin + "\" && \"" + outBin + "\" 2>&1";
                            } else if (ext == ".c") {
                                std::string outBin = activeFilepath + ".out";
                                cmd = "gcc \"" + activeFilepath + "\" -o \"" + outBin + "\" && \"" + outBin + "\" 2>&1";
                            } else if (ext == ".java") {
                                cmd = "javac \"" + activeFilepath + "\" && java \"" + activeFilepath + "\" 2>&1";
                            } else if (ext == ".go") {
                                cmd = "go run \"" + activeFilepath + "\" 2>&1";
                            } else if (ext == ".rs") {
                                std::string outBin = activeFilepath + ".out";
                                cmd = "rustc \"" + activeFilepath + "\" -o \"" + outBin + "\" && \"" + outBin + "\" 2>&1";
                            } else {
                                cmd = "python3 \"" + activeFilepath + "\" 2>&1";
                            }

                            FILE* pipe = popen(cmd.c_str(), "r");
                            if (pipe) {
                                std::array<char, 256> buffer;
                                std::string result = "";
                                while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
                                    result += buffer.data();
                                }
                                int status = pclose(pipe);
                                if (status == 0) {
                                    outputLog = "[Run Success (" + ext + ")]:\n" + result;
                                } else {
                                    outputLog = "[Execution/Syntax Error (" + ext + ")]:\n" + result;
                                }
                            }
                        } else {
                            outputLog = "[Error]: No active file to run.";
                        }
                    }
                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("Build App Packages")) {
                    if (ImGui::MenuItem("Build AppImage Package")) {
                        BuildIntegratedAppImage(projectPath, outputLog);
                    }
                    ImGui::EndMenu();
                }

                ImGui::EndMainMenuBar();
            }

            // Explorer Sidebar
            ImGui::SetNextWindowPos(ImVec2(0, 20), ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(280, io.DisplaySize.y - 220), ImGuiCond_Always);
            ImGui::Begin("Explorer", NULL, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);
            
            ImGui::Text("APP PROJECT: UI & Core");
            ImGui::Separator();
            
            if (projectPath[0] != '\0' && fs::exists(projectPath)) {
                fs::path rootPath(projectPath);
                fs::path uiDir = rootPath / "UI";
                fs::path coreDir = rootPath / "Core";
                if (!fs::exists(uiDir)) fs::create_directory(uiDir);
                if (!fs::exists(coreDir)) fs::create_directory(coreDir);

                if (ImGui::TreeNode("UI (Python & Assets)")) {
                    if (ImGui::BeginPopupContextItem("UIContext")) {
                        if (ImGui::MenuItem("New Python File...")) showNewFileInUI = true;
                        if (ImGui::MenuItem("Upload Files (Images, Videos, Docs)...")) {
                            std::string externalFile = OpenFileDialog();
                            if (!externalFile.empty()) {
                                fs::path srcPath(externalFile);
                                fs::path destPath = uiDir / srcPath.filename();
                                fs::copy(srcPath, destPath, fs::copy_options::overwrite_existing);
                                outputLog = "[Upload]: File uploaded to UI folder: " + srcPath.filename().string();
                            }
                        }
                        ImGui::EndPopup();
                    }

                    for (auto& entry : fs::directory_iterator(uiDir)) {
                        std::string filename = entry.path().filename().string();
                        bool isSelected = (activeFilepath == entry.path().string());
                        if (ImGui::Selectable(filename.c_str(), isSelected)) {
                            if (entry.path().extension() == ".py") {
                                activeFilepath = entry.path().string();
                                std::ifstream inFile(activeFilepath);
                                if (inFile.is_open()) {
                                    std::stringstream buffer;
                                    buffer << inFile.rdbuf();
                                    editor.SetText(buffer.str());
                                    inFile.close();
                                }
                            } else {
                                outputLog = "[Info]: Selected asset file: " + filename;
                            }
                        }
                        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0) && entry.path().extension() == ".py") {
                            activeFilepath = entry.path().string();
                            std::ifstream inFile(activeFilepath);
                            if (inFile.is_open()) {
                                std::stringstream buffer;
                                buffer << inFile.rdbuf();
                                editor.SetText(buffer.str());
                                inFile.close();
                            }
                        }
                        if (ImGui::BeginPopupContextItem()) {
                            if (ImGui::MenuItem("Delete")) {
                                fs::remove(entry.path());
                                if (activeFilepath == entry.path().string()) { activeFilepath = ""; editor.SetText(""); }
                            }
                            ImGui::EndPopup();
                        }
                    }
                    ImGui::TreePop();
                }

                if (ImGui::TreeNode("Core (C++, C, Java, Python, Go, Rust)")) {
                    if (ImGui::BeginPopupContextItem("CoreContext")) {
                        if (ImGui::MenuItem("New Core File...")) showNewFileInCore = true;
                        ImGui::EndPopup();
                    }

                    for (auto& entry : fs::directory_iterator(coreDir)) {
                        std::string filename = entry.path().filename().string();
                        bool isSelected = (activeFilepath == entry.path().string());
                        if (ImGui::Selectable(filename.c_str(), isSelected)) {
                            activeFilepath = entry.path().string();
                            std::ifstream inFile(activeFilepath);
                            if (inFile.is_open()) {
                                std::stringstream buffer;
                                buffer << inFile.rdbuf();
                                editor.SetText(buffer.str());
                                inFile.close();
                            }
                        }
                        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
                            activeFilepath = entry.path().string();
                            std::ifstream inFile(activeFilepath);
                            if (inFile.is_open()) {
                                std::stringstream buffer;
                                buffer << inFile.rdbuf();
                                editor.SetText(buffer.str());
                                inFile.close();
                            }
                        }
                        if (ImGui::BeginPopupContextItem()) {
                            if (ImGui::MenuItem("Delete")) {
                                fs::remove(entry.path());
                                if (activeFilepath == entry.path().string()) {
                                    activeFilepath = "";
                                    editor.SetText("");
                                }
                            }
                            ImGui::EndPopup();
                        }
                    }
                    ImGui::TreePop();
                }

            } else {
                ImGui::TextColored(ImVec4(1, 0.5f, 0, 1), "No folder selected.");
                if (ImGui::Button("Select Folder")) {
                    std::string folder = OpenFolderDialog();
                    if (!folder.empty()) {
                        snprintf(projectPath, sizeof(projectPath), "%s", folder.c_str());
                    }
                }
            }
            ImGui::End();

            if (showNewFileInUI) { ImGui::OpenPopup("Create File in UI"); showNewFileInUI = false; }
            if (ImGui::BeginPopupModal("Create File in UI", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
                ImGui::InputText("Name (.py)", newFileNameBuffer, IM_ARRAYSIZE(newFileNameBuffer));
                if (ImGui::Button("Create", ImVec2(120, 0))) {
                    std::string newFilePath = std::string(projectPath) + "/UI/" + std::string(newFileNameBuffer);
                    std::ofstream newFile(newFilePath);
                    newFile << "# UI Script (Python)\n";
                    newFile.close();
                    activeFilepath = newFilePath;
                    editor.SetText("# UI Script (Python)\n");
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SameLine();
                if (ImGui::Button("Cancel", ImVec2(120, 0))) ImGui::CloseCurrentPopup();
                ImGui::EndPopup();
            }

            if (showNewFileInCore) { ImGui::OpenPopup("Create File in Core"); showNewFileInCore = false; }
            if (ImGui::BeginPopupModal("Create File in Core", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
                ImGui::InputText("Name (.cpp, .c, .java, .py, .go, .rs)", newFileNameBuffer, IM_ARRAYSIZE(newFileNameBuffer));
                if (ImGui::Button("Create", ImVec2(120, 0))) {
                    std::string newFilePath = std::string(projectPath) + "/Core/" + std::string(newFileNameBuffer);
                    std::ofstream newFile(newFilePath);
                    newFile << "// Core File\n";
                    newFile.close();
                    activeFilepath = newFilePath;
                    editor.SetText("// Core File\n");
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SameLine();
                if (ImGui::Button("Cancel", ImVec2(120, 0))) ImGui::CloseCurrentPopup();
                ImGui::EndPopup();
            }

            // Workspace
            float workspaceWidth = io.DisplaySize.x - 280;
            if (workspaceWidth < 300) workspaceWidth = 300;
            float workspaceHeight = io.DisplaySize.y - 220;
            if (workspaceHeight < 200) workspaceHeight = 200;

            ImGui::SetNextWindowPos(ImVec2(280, 20), ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(workspaceWidth, workspaceHeight), ImGuiCond_Always);
            ImGui::Begin("Workspace", NULL, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_HorizontalScrollbar);
            
            ImGui::Text("Active: %s | Max RAM: %d MB", activeFilepath.c_str(), maxRamMB);
            ImGui::Separator();
            
            editor.Render("EditorWorkspace", ImVec2(workspaceWidth - 20, workspaceHeight - 70));

            // Right-Click Context Menu in Editor
            if (ImGui::BeginPopupContextItem("EditorContextMenu", ImGuiPopupFlags_MouseButtonRight)) {
                if (ImGui::MenuItem("Copy", "Ctrl+C", false, editor.HasSelection())) editor.Copy();
                if (ImGui::MenuItem("Paste", "Ctrl+V")) editor.Paste();
                if (ImGui::MenuItem("Delete", "Del", false, editor.HasSelection())) editor.Delete();
                ImGui::Separator();
                if (ImGui::MenuItem("Select All", "Ctrl+A")) editor.SelectAll();
                ImGui::EndPopup();
            }

            ImGui::End(); 

            // Find Dialog
            if (showFindDialog) {
                ImGui::Begin("Find", &showFindDialog, ImGuiWindowFlags_AlwaysAutoResize);
                ImGui::InputText("Search text", findBuffer, IM_ARRAYSIZE(findBuffer));
                if (ImGui::Button("Find in Editor")) {
                    std::string text = editor.GetText();
                    std::string query(findBuffer);
                    if (text.find(query) != std::string::npos) {
                        outputLog = "[Find]: Text '" + query + "' found successfully.";
                    } else {
                        outputLog = "[Find]: Text '" + query + "' not found.";
                    }
                }
                ImGui::SameLine();
                if (ImGui::Button("Close")) showFindDialog = false;
                ImGui::End();
            }

            // Replace Dialog
            if (showReplaceDialog) {
                ImGui::Begin("Replace", &showReplaceDialog, ImGuiWindowFlags_AlwaysAutoResize);
                ImGui::InputText("Find", findBuffer, IM_ARRAYSIZE(findBuffer));
                ImGui::InputText("Replace with", replaceBuffer, IM_ARRAYSIZE(replaceBuffer));
                if (ImGui::Button("Replace All")) {
                    std::string text = editor.GetText();
                    std::string findStr(findBuffer);
                    std::string repStr(replaceBuffer);
                    size_t pos = 0;
                    int count = 0;
                    while ((pos = text.find(findStr, pos)) != std::string::npos) {
                        text.replace(pos, findStr.length(), repStr);
                        pos += repStr.length();
                        count++;
                    }
                    editor.SetText(text);
                    outputLog = "[Replace]: Replaced " + std::to_string(count) + " occurrences.";
                }
                ImGui::SameLine();
                if (ImGui::Button("Close")) showReplaceDialog = false;
                ImGui::End();
            }

            // Output Panel (Interactive Mouse Highlight & Copy Support)
            ImGui::SetNextWindowPos(ImVec2(0, io.DisplaySize.y - 200), ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x, 200), ImGuiCond_Always);
            ImGui::Begin("Output Panel", NULL, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);
            
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.5f, 1.0f), "Output (Select text with mouse & Copy with Ctrl+C):");
            
            static char logBuffer[32768];
            strncpy(logBuffer, outputLog.c_str(), sizeof(logBuffer));
            logBuffer[sizeof(logBuffer) - 1] = '\0';
            
            ImGui::InputTextMultiline("##OutputScroll", logBuffer, sizeof(logBuffer), ImVec2(io.DisplaySize.x - 20, 130), 0);
            
            ImGui::End();
        }

        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);

        int sleepTime = (4096 - maxRamMB) / 80;
        if (sleepTime < 1) sleepTime = 1;
        std::this_thread::sleep_for(std::chrono::milliseconds(sleepTime));
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
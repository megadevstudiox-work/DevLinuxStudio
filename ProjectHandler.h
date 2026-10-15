#pragma once
#include <string>
#include <fstream>
#include "TextEditor.h"

inline void CreateProjectFile(const std::string& path, int lang) {
    std::string filePath = path + (lang == 0 ? "/main.cpp" : "/main.py");
    std::ofstream file(filePath);
    if (lang == 0) file << "#include <iostream>\n\nint main() {\n    std::cout << \"Hello World!\";\n    return 0;\n}";
    else file << "print('Hello World!')";
    file.close();
}
char terminalOutput[4096] = "Terminal ready..."; 

inline void SaveFile(const std::string& path, TextEditor& editor) {
    std::ofstream file(path);
    file << editor.GetText();
    file.close();
}
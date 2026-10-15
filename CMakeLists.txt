include_directories(${CMAKE_SOURCE_DIR})
cmake_minimum_required(VERSION 3.10)
project(DevLinuxStudio)

set(CMAKE_CXX_STANDARD 17)

# Gjej paketat e nevojshme (OpenGL, GLFW, etj.)
find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)

# Shto skedarët burim të ImGui
set(IMGUI_DIR imgui)
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

# Shto ekzekutuesin e projektit
add_executable(DevLinuxStudio
    main.cpp
    TextEditor.cpp
    ${IMGUI_SOURCES}
)

# Lidh direktoritë e skedarëve .h
target_include_directories(DevLinuxStudio PRIVATE
    .
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)

# Lidh libraritë e sistemit (GLFW, OpenGL, pthread, dl, etj.)
target_link_libraries(DevLinuxStudio PRIVATE
    glfw
    OpenGL::GL
    dl
    pthread
)
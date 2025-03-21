cmake_minimum_required(VERSION 3.24)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# vcpkg toolchain
if(DEFINED ENV{VCPKG_ROOT})
    cmake_path(SET CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
    # set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
else()
    message(SEND_ERROR "Vcpkg toolchain not configured. Either set VCPKG_ROOT environment variable or pass -DCMAKE_TOOLCHAIN_FILE=VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake to cmake")
endif()
# set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake")

# PROJECT SETUP
set(PROJECT DynamicFonts)
set(PROJECT_LIB ${PROJECT}_lib)
project(${PROJECT} CXX)


# PACKAGES 
find_package(gl3w CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glslang CONFIG REQUIRED)
find_package(msdfgen CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(Freetype REQUIRED)

# Dear ImGui
set(IMGUI_DIR imgui)

# LIBRARY
set(SrcDir "${CMAKE_CURRENT_SOURCE_DIR}/src")
file(GLOB_RECURSE Sources_SRC CONFIGURE_DEPENDS
     "${SrcDir}/*.h"
     "${SrcDir}/*.cpp"
)

add_library(${PROJECT_LIB} STATIC
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/misc/cpp/imgui_stdlib.cpp
    ${IMGUI_DIR}/misc/freetype/imgui_freetype.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp 
    ${Sources_SRC}
    ${CommonLib_SRC}
)

target_include_directories(${PROJECT_LIB} PUBLIC ${IMGUI_DIR})
target_include_directories(${PROJECT_LIB} PUBLIC ${IMGUI_DIR}/misc/cpp)

target_link_libraries(${PROJECT_LIB} PUBLIC 
    glfw
    glslang::glslang
    unofficial::gl3w::gl3w
    fmt::fmt 
    spdlog::spdlog
    Freetype::Freetype
    msdfgen::msdfgen
)


target_compile_definitions(${PROJECT_LIB} PUBLIC IMGUI_ENABLE_FREETYPE IMGUI_DEFINE_MATH_OPERATORS IMGUI_DISABLE_OBSOLETE_FUNCTIONS NOMINMAX)

set_property(TARGET ${PROJECT_LIB} PROPERTY COMPILE_WARNING_AS_ERROR OFF)

# EXECUTABLE
add_executable(${PROJECT} ${SOURCES}
    main.cpp
)

target_link_libraries(${PROJECT} PRIVATE
    ${PROJECT_LIB}
)

target_link_libraries(${PROJECT} PRIVATE OpenGL32)
# =============================================================================
# Chapix - Divine SandBox
# CMake Build System
# =============================================================================
cmake_minimum_required(VERSION 3.16)
project(Chapix VERSION 1.0 LANGUAGES CXX)

# --- C++ Standard ---
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Export compile_commands.json for VS Code / Clangd ---
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# =============================================================================
# Source Files (organized by module)
# =============================================================================

# Core
set(CORE_SOURCES
    src/core/AudioManager.cpp
    src/core/CrashHandler.cpp
    src/core/SaveManager.cpp
)

# Graphics
set(GRAPHICS_SOURCES
    src/graphics/WorldRenderer.cpp
)

# Resources
set(RESOURCES_SOURCES
    src/resources/ResourceManager.cpp
)

# Simulation
set(SIMULATION_SOURCES
    src/simulation/BuildingManager.cpp
    src/simulation/Citizen.cpp
    src/simulation/CitizenAI.cpp
    src/simulation/City.cpp
    src/simulation/CityManager.cpp
    src/simulation/Kingdom.cpp
    src/simulation/KingdomManager.cpp
    src/simulation/SimulationManager.cpp
)

# UI
set(UI_SOURCES
    src/ui/UICityPopup.cpp
    src/ui/UIHumanPopup.cpp
    src/ui/UIMainMenu.cpp
    src/ui/UIManager.cpp
    src/ui/UISavePopup.cpp
    src/ui/UIToolbar.cpp
)

# Utils
set(UTILS_SOURCES
    src/utils/Noise.cpp
)

# World
set(WORLD_SOURCES
    src/world/World.cpp
)

# =============================================================================
# Executable
# =============================================================================
add_executable(${PROJECT_NAME}
    src/main.cpp
    ${CORE_SOURCES}
    ${GRAPHICS_SOURCES}
    ${RESOURCES_SOURCES}
    ${SIMULATION_SOURCES}
    ${UI_SOURCES}
    ${UTILS_SOURCES}
    ${WORLD_SOURCES}
)

# =============================================================================
# Raylib (local pre-built library)
# =============================================================================
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/lib/raylib/include
)

target_link_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/lib/raylib/lib
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    raylib
    opengl32
    gdi32
    winmm
)

# --- Static linking (match original build.bat behavior) ---
target_link_options(${PROJECT_NAME} PRIVATE -static)

# =============================================================================
# Output Configuration
# =============================================================================
# Put the final executable in bin/ (same as build.bat)
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin
    OUTPUT_NAME "ChapiX"
)

# For multi-config generators (not typical for MinGW, but just in case)
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY_DEBUG   ${CMAKE_SOURCE_DIR}/bin
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/bin
)

# =============================================================================
# Build Type Defaults
# =============================================================================
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Build type not specified, defaulting to Release")
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()

@echo off
if not exist "bin" mkdir bin

if exist "compiler\w64devkit\bin" (
    echo Configurando ambiente do compilador local...
    set "PATH=%CD%\compiler\w64devkit\bin;%PATH%"
)

echo Compiling...
g++ src/main.cpp src/core/SaveManager.cpp src/core/AudioManager.cpp src/core/CrashHandler.cpp src/world/World.cpp src/utils/Noise.cpp src/resources/ResourceManager.cpp src/graphics/WorldRenderer.cpp src/ui/UIManager.cpp src/ui/UICityPopup.cpp src/ui/UIHumanPopup.cpp src/ui/UIToolbar.cpp src/ui/UISavePopup.cpp src/ui/UIMainMenu.cpp src/simulation/Citizen.cpp src/simulation/City.cpp src/simulation/Kingdom.cpp src/simulation/SimulationManager.cpp src/simulation/CitizenAI.cpp src/simulation/CityManager.cpp src/simulation/BuildingManager.cpp src/simulation/KingdomManager.cpp -o bin/game.exe -I lib/raylib/include -L lib/raylib/lib -lraylib -lopengl32 -lgdi32 -lwinmm -static

if %errorlevel% neq 0 (
    echo Compilation Failed!

    exit /b %errorlevel%
)

echo Compilation Success! Run bin\game.exe to play.

@echo off
if not exist "bin" mkdir bin

if exist "compiler\w64devkit\bin" (
    echo Configurando ambiente do compilador local...
    set "PATH=%CD%\compiler\w64devkit\bin;%PATH%"
)

echo Compiling...
g++ src/main.cpp src/world/World.cpp src/utils/Noise.cpp src/resources/ResourceManager.cpp src/graphics/WorldRenderer.cpp src/ui/UIManager.cpp src/simulation/Citizen.cpp src/simulation/City.cpp src/simulation/Kingdom.cpp src/simulation/SimulationManager.cpp -o bin/game.exe -I lib/raylib/include -L lib/raylib/lib -lraylib -lopengl32 -lgdi32 -lwinmm -static

if %errorlevel% neq 0 (
    echo Compilation Failed!
    pause
    exit /b %errorlevel%
)

echo Compilation Success! Run bin\game.exe to play.

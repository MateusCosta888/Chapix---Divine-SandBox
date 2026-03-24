@echo off
REM =============================================================================
REM Chapix - CMake Build Script
REM Usage:
REM   build_cmake.bat          -> Compila em modo Release (otimizado)
REM   build_cmake.bat debug    -> Compila em modo Debug (com symbols)
REM   build_cmake.bat clean    -> Limpa a pasta de build
REM =============================================================================

REM --- Setup local compiler if available ---
if exist "compiler\w64devkit\bin" (
    echo [CMAKE] Configurando compilador local...
    set "PATH=%CD%\compiler\w64devkit\bin;%PATH%"
)

REM --- Handle "clean" argument ---
if "%1"=="clean" (
    echo [CMAKE] Limpando pasta de build...
    if exist "build" rmdir /s /q build
    echo [CMAKE] Limpo!
    exit /b 0
)

REM --- Setup paths ---
set "PATH=C:\Program Files\CMake\bin;%PATH%"

REM --- Build Type ---
set BUILD_TYPE=Release
if /I "%1"=="debug" set BUILD_TYPE=Debug

REM --- Create build directory and configure (only on first run) ---
if exist "build\CMakeCache.txt" goto build_step

echo [CMAKE] Configurando projeto pela primeira vez...
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=%BUILD_TYPE% -DCMAKE_MAKE_PROGRAM="%CD%\compiler\w64devkit\bin\make.exe"
if errorlevel 1 (
    echo [CMAKE] Falha na configuracao! Verifique se possui o CMake instalado (winget install Kitware.CMake).
    exit /b 1
)

:build_step
REM --- Build ---
echo [CMAKE] Compilando em modo %BUILD_TYPE%...
cmake --build build -j %NUMBER_OF_PROCESSORS%

if %errorlevel% neq 0 (
    echo [CMAKE] Falha na compilacao!
    exit /b %errorlevel%
)

echo [CMAKE] Sucesso! Execute bin\ChapiX.exe para jogar.

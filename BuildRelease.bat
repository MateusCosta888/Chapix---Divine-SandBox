@echo off
setlocal enabledelayedexpansion

:: ============================================================================
:: CHAPIX - BUILD RELEASE SYSTEM
:: Este script compila o jogo para a versao final de distribuicao.
:: ============================================================================

echo.
echo  ########################################
echo  #                                      #
echo  #      CHAPIX: DIVINE SANDBOX          #
echo  #        Build Release System          #
echo  #                                      #
echo  ########################################
echo.

:: 1. Criar pasta de Release
echo [1/5] Preparando diretorios...
if exist "Release" (
    echo Limpando versao anterior...
    rd /s /q "Release"
)
mkdir "Release"

:: 2. Configurar ambiente do compilador
if exist "compiler\w64devkit\bin" (
    echo [2/5] Configurando compilador local...
    set "PATH=%CD%\compiler\w64devkit\bin;%PATH%"
) else (
    echo [!] Compilador local nao encontrado. Tentando usar compilador do sistema...
)

:: 3. Compilar Recursos (Icone e Metadados)
echo [3/5] Compilando Recursos (Icone e Metadados)...
windres resource.rc -O coff -o resource.res
if %errorlevel% neq 0 (
    echo [ERRO] Falha ao compilar resource.rc
    pause
    exit /b %errorlevel%
)

:: 4. Compilacao Principal (Modo Release)
echo [4/5] Compilando Codigo Fonte (Modo Release)...
echo Isso pode levar alguns segundos, por favor aguarde...

:: Lista de arquivos (copiada do build original para garantir compatibilidade)
set "SOURCES=src/main.cpp src/core/SaveManager.cpp src/core/AudioManager.cpp src/core/CrashHandler.cpp src/world/World.cpp src/utils/Noise.cpp src/resources/ResourceManager.cpp src/graphics/WorldRenderer.cpp src/ui/UIManager.cpp src/ui/UICityPopup.cpp src/ui/UIHumanPopup.cpp src/ui/UIToolbar.cpp src/ui/UISavePopup.cpp src/ui/UIMainMenu.cpp src/simulation/Citizen.cpp src/simulation/City.cpp src/simulation/Kingdom.cpp src/simulation/SimulationManager.cpp src/simulation/CitizenAI.cpp src/simulation/CityManager.cpp src/simulation/BuildingManager.cpp src/simulation/KingdomManager.cpp"

:: Flags:
:: -mwindows: Esconde o console de debug
:: -static: Linkagem estatica (nao precisa de DLLs externas)
:: -O3: Otimizacao maxima de performance
:: -s: Strip (remove informacoes de debug, diminui o tamanho do .exe)
:: -DRELEASE: Macro para desativar logs no main.cpp
g++ %SOURCES% resource.res -o Release/ChapiX.exe -I lib/raylib/include -L lib/raylib/lib -lraylib -lopengl32 -lgdi32 -lwinmm -mwindows -static -O3 -s -DRELEASE

if %errorlevel% neq 0 (
    echo.
    echo [ERRO] A compilacao falhou! Verifique os erros acima.
    del resource.res
    pause
    exit /b %errorlevel%
)

:: 5. Copiar Assets
echo [5/5] Empacotando Assets...
if exist "assets" (
    xcopy /s /e /i /y "assets" "Release\assets" > nul
) else (
    echo [AVISO] Pasta 'assets' nao encontrada! O jogo pode nao funcionar corretamente.
)

:: Limpeza
del resource.res

echo.
echo ============================================================
echo  SUCESSO: Jogo pronto para distribuicao!
echo ============================================================
echo.
echo  A pasta 'Release' agora contem tudo o que voce precisa:
echo  1. ChapiX.exe (O jogo finalizado)
echo  2. assets/ (Todas as imagens e sons)
echo.
echo  Voce pode renomear a pasta 'Release' para 'ChapiX_V1.0'
echo  e compactar em .ZIP para enviar aos jogadores.
echo.
echo ============================================================
pause

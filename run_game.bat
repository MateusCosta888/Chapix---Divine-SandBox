@echo off
cd /d "%~dp0"
if exist "bin\ChapiX.exe" (
    echo Iniciando o ChapiX...
    start "" "bin\ChapiX.exe"
) else (
    echo [ERRO] bin\ChapiX.exe nao foi encontrado! Por favor, compile o jogo primeiro rodando build.bat
    pause
)

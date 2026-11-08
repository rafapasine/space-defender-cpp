@echo off
setlocal
rem ==========================================================
rem  Compila o main.cpp (Space Defender C++) usando o MSYS2 (UCRT64)
rem  Coloque este arquivo na MESMA pasta do main.cpp e de dois cliques.
rem ==========================================================
cd /d "%~dp0"

set "MSYS=C:\msys64"
set "UCRT=%MSYS%\ucrt64"

if not exist main.cpp (
    echo [ERRO] main.cpp nao encontrado nesta pasta: %CD%
    goto fim
)
if not exist "%UCRT%\bin\g++.exe" (
    echo [ERRO] g++ do MSYS2 nao encontrado em %UCRT%\bin
    goto fim
)

rem Usa SOMENTE o g++ do MSYS2 (ignora o do Dev-C++)
set "PATH=%UCRT%\bin;%MSYS%\usr\bin;%PATH%"
echo Usando: %UCRT%\bin\g++.exe
echo.

rem 1) Instala a Raylib no MSYS2 se ainda nao estiver instalada
if not exist "%UCRT%\include\raylib.h" (
    echo Raylib nao encontrada. Instalando pelo pacman, aguarde...
    "%MSYS%\usr\bin\bash.exe" -lc "pacman -S --needed --noconfirm mingw-w64-ucrt-x86_64-raylib"
    if not exist "%UCRT%\include\raylib.h" (
        echo [ERRO] Nao foi possivel instalar a Raylib automaticamente.
        echo Abra o "MSYS2 UCRT64" e rode:  pacman -S mingw-w64-ucrt-x86_64-raylib
        goto fim
    )
    echo Raylib instalada.
    echo.
)

rem 2) Fecha o jogo se ele ainda estiver aberto (evita "Permission denied")
taskkill /f /im jogo_espacial.exe >nul 2>&1

set "FLAGS=-std=c++17 -O2 -mwindows"
set "WINLIBS=-lopengl32 -lgdi32 -lwinmm"

rem 3) Tentativa A: exe independente (estatico)
echo Compilando (tentativa 1 de 3)...
g++ main.cpp -o jogo_espacial.exe %FLAGS% -static -lraylib %WINLIBS%
if not errorlevel 1 goto ok_static

rem    Tentativa B: estatico, com GLFW separado (alguns builds da Raylib exigem)
echo.
echo Compilando (tentativa 2 de 3)...
g++ main.cpp -o jogo_espacial.exe %FLAGS% -static -lraylib -lglfw3 %WINLIBS%
if not errorlevel 1 goto ok_static

rem    Tentativa C: dinamico (precisa das DLLs do MSYS2)
echo.
echo Compilando (tentativa 3 de 3)...
g++ main.cpp -o jogo_espacial.exe %FLAGS% -lraylib %WINLIBS%
if errorlevel 1 (
    echo.
    echo [FALHOU] Tire um print de TODAS as mensagens acima e envie para analise.
    goto fim
)
echo.
echo [OK] jogo_espacial.exe criado (versao dinamica).
echo Ele so abre por este .bat ou em PCs com %UCRT%\bin no PATH.
start "" jogo_espacial.exe
goto fim

:ok_static
echo.
echo [OK] jogo_espacial.exe criado em %CD%
echo Este .exe funciona sozinho: pode copiar para qualquer PC Windows.
start "" jogo_espacial.exe

:fim
echo.
pause

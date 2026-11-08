@echo off
setlocal
rem ==========================================================
rem  Compila o main.cpp (Space Defender C++) em jogo_espacial.exe
rem  Coloque este arquivo na MESMA pasta do main.cpp e de dois cliques.
rem ==========================================================
cd /d "%~dp0"

if not exist main.cpp (
    echo [ERRO] main.cpp nao encontrado nesta pasta: %CD%
    goto fim
)

rem 1) Usa o g++ que vem com o instalador da Raylib (evita conflito com outro MinGW)
if exist "C:\raylib\w64devkit\bin\g++.exe" (
    set "PATH=C:\raylib\w64devkit\bin;%PATH%"
)
where g++ >nul 2>&1
if errorlevel 1 (
    echo [ERRO] g++ nao encontrado. Instale a Raylib pelo instalador oficial em C:\raylib
    goto fim
)
echo Usando o compilador:
where g++
echo.

rem 2) Fecha o jogo se ele ainda estiver aberto (evita "Permission denied")
taskkill /f /im jogo_espacial.exe >nul 2>&1

rem 3) Procura automaticamente raylib.h e libraylib.a dentro de C:\raylib
set "RAYINC="
set "RAYLIB="
if exist C:\raylib (
    for /r C:\raylib %%f in (libraylib.a) do if exist "%%f" if not defined RAYLIB set "RAYLIB=%%~dpf"
    for /r C:\raylib %%f in (raylib.h)    do if exist "%%f" if not defined RAYINC set "RAYINC=%%~dpf"
)

set "EXTRA="
if defined RAYINC set "EXTRA=-I"%RAYINC:~0,-1%""
if defined RAYLIB set "EXTRA=%EXTRA% -L"%RAYLIB:~0,-1%""
echo raylib.h    em: %RAYINC%
echo libraylib.a em: %RAYLIB%
echo.

rem 4) Compila
echo Compilando...
g++ main.cpp -o jogo_espacial.exe -std=c++17 -O2 %EXTRA% -lraylib -lopengl32 -lgdi32 -lwinmm -static -mwindows
if errorlevel 1 (
    echo.
    echo [FALHOU] Copie TODAS as mensagens acima e envie para analise.
    goto fim
)

echo.
echo [OK] jogo_espacial.exe criado em %CD%
start "" jogo_espacial.exe

:fim
echo.
pause
@echo off
setlocal
cd /d "%~dp0"

if exist "bin" (
    rmdir /s /q "bin" 2>nul
    if exist "bin" (
        echo bin locked — close GW2 first
        exit /b 1
    )
)

if exist "obj" rmdir /s /q "obj"
exit /b 0

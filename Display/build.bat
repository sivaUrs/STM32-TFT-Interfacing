@echo off
REM Build script for STM32F407 project
cd /d "%~dp0001_Vibgyor_bars\Debug"

REM Try to find make in common locations
if exist "C:\Program Files\GNU Tools\bin\make.exe" (
    "C:\Program Files\GNU Tools\bin\make.exe" %*
    exit /b !errorlevel!
)

if exist "C:\MinGW\bin\make.exe" (
    "C:\MinGW\bin\make.exe" %*
    exit /b !errorlevel!
)

if exist "C:\msys64\usr\bin\make.exe" (
    "C:\msys64\usr\bin\make.exe" %*
    exit /b !errorlevel!
)

REM If make not found, show error
echo Error: GNU Make not found. Please install MinGW or GNU Tools for ARM Embedded Processors
exit /b 1

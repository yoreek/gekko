@echo off
REM Flashes the firmware via standalone esptool.exe (no Python, no browser needed).
REM Download esptool.exe and place it next to this file:
REM   https://github.com/espressif/esptool/releases
REM
REM Usage: flash.bat [COM3]

setlocal
cd /d "%~dp0"

if not exist esptool.exe (
  echo esptool.exe not found. Download it from:
  echo   https://github.com/espressif/esptool/releases
  echo and place it next to flash.bat.
  exit /b 1
)

set PORT_ARG=
if not "%~1"=="" set PORT_ARG=--port %1

esptool.exe --chip esp32 --baud 921600 %PORT_ARG% write_flash -z ^
  --flash_mode dio --flash_freq 40m --flash_size detect ^
  0x1000 bootloader.bin ^
  0x8000 partitions.bin ^
  0x10000 firmware.bin ^
  0x383000 littlefs.bin

pause

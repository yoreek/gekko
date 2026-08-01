@echo off
REM Flashes the firmware via standalone esptool.exe (no Python, no browser needed).
REM "default" is the recommended build (no BLE provisioning); "ble" adds BLE WiFi
REM provisioning and its setup-button GPIO reservation, at the cost of flash/RAM.
REM CHIP is one of esp32 (default), esp32s2, esp32s3, esp32c3, esp32c6 -- match the
REM board you actually have; "ble" only exists for esp32/esp32s3/esp32c3.
REM Download esptool.exe and place it next to this file:
REM   https://github.com/espressif/esptool/releases
REM
REM Usage: flash.bat [default^|ble] [CHIP] [PORT] [all^|bootloader^|partitions^|firmware^|littlefs]
REM The target can be passed without a port, for example: flash.bat littlefs

setlocal EnableDelayedExpansion
cd /d "%~dp0"

set "VARIANT_DIR=."
if /i "%~1"=="default" shift
if /i "%~1"=="ble" (
  set "VARIANT_DIR=ble"
  shift
)

set "CHIP=esp32"
if /i "%~1"=="esp32" (set "CHIP=esp32" & shift)
if /i "%~1"=="esp32s2" (set "CHIP=esp32s2" & shift)
if /i "%~1"=="esp32s3" (set "CHIP=esp32s3" & shift)
if /i "%~1"=="esp32c3" (set "CHIP=esp32c3" & shift)
if /i "%~1"=="esp32c6" (set "CHIP=esp32c6" & shift)
set "BUNDLE_DIR=!VARIANT_DIR!\!CHIP!"

if not exist "!BUNDLE_DIR!\flash-layout.env" (
  echo !BUNDLE_DIR!\flash-layout.env not found.
  exit /b 1
)
for /f "usebackq tokens=1,* delims==" %%A in ("!BUNDLE_DIR!\flash-layout.env") do set "%%A=%%B"

if not exist esptool.exe (
  echo esptool.exe not found. Download it from:
  echo   https://github.com/espressif/esptool/releases
  echo and place it next to flash.bat.
  exit /b 1
)

set "TARGET=all"
set "PORT_ARG="
if /i "%~1"=="all" set "TARGET=all"
if /i "%~1"=="bootloader" set "TARGET=bootloader"
if /i "%~1"=="partitions" set "TARGET=partitions"
if /i "%~1"=="firmware" set "TARGET=firmware"
if /i "%~1"=="littlefs" set "TARGET=littlefs"
if not "%~1"=="" if "!TARGET!"=="all" if /i not "%~1"=="all" set "PORT_ARG=--port %~1"
if not "%~2"=="" set "TARGET=%~2"

if /i "!TARGET!"=="all" (
  set "FLASH_FILE=!MERGED_FILE!"
  set "FLASH_OFFSET=!MERGED_OFFSET!"
  echo WARNING: the combined image erases devdata; use a selective target to preserve it.
) else if /i "!TARGET!"=="bootloader" (
  set "FLASH_FILE=!BOOTLOADER_FILE!"
  set "FLASH_OFFSET=!BOOTLOADER_OFFSET!"
) else if /i "!TARGET!"=="partitions" (
  set "FLASH_FILE=!PARTITIONS_FILE!"
  set "FLASH_OFFSET=!PARTITIONS_OFFSET!"
) else if /i "!TARGET!"=="firmware" (
  set "FLASH_FILE=!FIRMWARE_FILE!"
  set "FLASH_OFFSET=!FIRMWARE_OFFSET!"
) else if /i "!TARGET!"=="littlefs" (
  set "FLASH_FILE=!LITTLEFS_FILE!"
  set "FLASH_OFFSET=!LITTLEFS_OFFSET!"
) else (
  echo Unknown target: !TARGET!
  exit /b 2
)

esptool.exe --chip !CHIP! --baud 921600 !PORT_ARG! write_flash -z ^
  --flash_mode keep --flash_freq keep --flash_size detect ^
  !FLASH_OFFSET! "!BUNDLE_DIR!\!FLASH_FILE!"

pause

@echo off
echo =====================================================================
echo Flashing STM32F407 Pure Bare-Metal Blinky via OpenOCD & ST-LINK
echo =====================================================================

set OPENOCD="C:\openocd\bin\openocd.exe"

%OPENOCD% -f board/stm32f4discovery.cfg -c "program blinky.elf verify reset exit"

if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Flashing failed! Ensure board is plugged in via ST-LINK Mini-USB port CN1.
    exit /b %ERRORLEVEL%
)

echo =====================================================================
echo Flashing Complete! Green LED (PD12) should now be blinking.
echo =====================================================================

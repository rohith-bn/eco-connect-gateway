@echo off
echo =====================================================================
echo Compiling STM32F407 Pure Bare-Metal Blinky (No HAL, No RTOS)
echo Author: Rohith B Narasimhamurthy
echo =====================================================================

set CC="C:\Program Files (x86)\Arm GNU Toolchain arm-none-eabi\14.2 rel1\bin\arm-none-eabi-gcc.exe"
set OBJCOPY="C:\Program Files (x86)\Arm GNU Toolchain arm-none-eabi\14.2 rel1\bin\arm-none-eabi-objcopy.exe"
set SIZE="C:\Program Files (x86)\Arm GNU Toolchain arm-none-eabi\14.2 rel1\bin\arm-none-eabi-size.exe"

%CC% -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -O0 -Wall -Wextra -T linker.ld -nostdlib startup.c main.c -Wl,-Map=blinky.map -o blinky.elf

if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Compilation failed!
    exit /b %ERRORLEVEL%
)

%OBJCOPY% -O binary blinky.elf blinky.bin
%SIZE% blinky.elf

echo =====================================================================
echo Build Successful! 
echo Binary generated: blinky.bin (Ready to flash via ST-Link / OpenOCD)
echo =====================================================================

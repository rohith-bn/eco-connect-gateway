# ==============================================================================
# Pure Bare-Metal Makefile for STM32F407 (ARM Cortex-M4)
# Author: Rohith B Narasimhamurthy
# Target: STM32F407G-DISC1
# ==============================================================================

# Target project name
TARGET = blinky

# Toolchain definitions
CC      = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
OBJDUMP = arm-none-eabi-objdump
SIZE    = arm-none-eabi-size
OPENOCD = C:/openocd/bin/openocd.exe

# Target hardware architecture flags
MCU_FLAGS = -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16

# Compiler flags for Blinky (-O0 for clear debugging)
CFLAGS = $(MCU_FLAGS) -O0 -Wall -Wextra -g3 -gdwarf-2 -nostdlib \
         -fdata-sections -ffunction-sections

# Linker script and flags
LDSCRIPT = linker.ld
LDFLAGS  = $(MCU_FLAGS) -T $(LDSCRIPT) -nostdlib \
           -Wl,--gc-sections -Wl,-Map=$(TARGET).map

# Source files and Object files for Blinky
SRCS = startup.c main.c
OBJS = $(SRCS:.c=.o)

# Volatile Demo Target Definitions (-O2 optimization to demonstrate loop stripping)
VOLATILE_DIR    = learning_experiments/01_volatile_deep_dive
VOLATILE_TARGET = $(VOLATILE_DIR)/volatile_demo
VOLATILE_CFLAGS = $(MCU_FLAGS) -O2 -Wall -Wextra -g3 -gdwarf-2 -nostdlib \
                  -fdata-sections -ffunction-sections
VOLATILE_LDFLAGS= $(MCU_FLAGS) -T $(LDSCRIPT) -nostdlib \
                  -Wl,--gc-sections -Wl,-Map=$(VOLATILE_TARGET).map

.PHONY: all clean flash disasm size volatile_demo

all: $(TARGET).elf $(TARGET).bin $(TARGET).asm size

# Compile C source files into relocatable object files (.o)
%.o: %.c
	@echo [CC] $<
	$(CC) $(CFLAGS) -c $< -o $@

# Link object files into final ELF container
$(TARGET).elf: $(OBJS) $(LDSCRIPT)
	@echo [LD] $@
	$(CC) $(OBJS) $(LDFLAGS) -o $@

# Convert ELF to raw flat binary for flash memory
$(TARGET).bin: $(TARGET).elf
	@echo [OBJCOPY] $@
	$(OBJCOPY) -O binary $< $@

# Disassemble ELF to human-readable assembly for verification
$(TARGET).asm: $(TARGET).elf
	@echo [OBJDUMP] $@
	$(OBJDUMP) -d -S $< > $@

# Print memory footprint
size: $(TARGET).elf
	@echo ===================================================
	@echo Firmware Memory Consumption:
	@$(SIZE) $(TARGET).elf
	@echo ===================================================

# ------------------------------------------------------------------------------
# Volatile Demo Build Target (Compiled with -O2 + DWARF debug symbols)
# ------------------------------------------------------------------------------
volatile_demo: $(VOLATILE_TARGET).elf

$(VOLATILE_DIR)/volatile_demo.o: $(VOLATILE_DIR)/volatile_demo.c
	@echo [CC -O2] $<
	$(CC) $(VOLATILE_CFLAGS) -c $< -o $@

$(VOLATILE_TARGET).elf: startup.o $(VOLATILE_DIR)/volatile_demo.o $(LDSCRIPT)
	@echo [LD -O2] $@
	$(CC) startup.o $(VOLATILE_DIR)/volatile_demo.o $(VOLATILE_LDFLAGS) -o $@
	@echo ===================================================
	@echo Volatile Demo Memory Footprint:
	@$(SIZE) $@
	@echo ===================================================

# Flash firmware onto STM32F407 silicon via OpenOCD
flash: $(TARGET).elf
	@echo [FLASH] Programming STM32F407 via OpenOCD...
	"$(OPENOCD)" -f board/stm32f4discovery.cfg -c "program $(TARGET).elf verify reset exit"

# Clean build artifacts
clean:
	@echo [CLEAN] Removing build artifacts...
	-cmd /c del /q /f *.o *.elf *.bin *.map *.asm 2>nul || rm -f *.o *.elf *.bin *.map *.asm
	-cmd /c del /q /f $(subst /,\,$(VOLATILE_DIR))\*.o $(subst /,\,$(VOLATILE_DIR))\*.elf $(subst /,\,$(VOLATILE_DIR))\*.map 2>nul || rm -f $(VOLATILE_DIR)/*.o $(VOLATILE_DIR)/*.elf $(VOLATILE_DIR)/*.map

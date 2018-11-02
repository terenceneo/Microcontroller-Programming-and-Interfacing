################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include subdir.mk
-include src/subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all: LPC1769_asm_basic.axf

# Tool invocations
LPC1769_asm_basic.axf: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: MCU Linker'
	arm-none-eabi-gcc -nostdlib -Tbss 0x10004000 -Xlinker -Map=LPC1769_asm_basic.map -Xlinker --gc-sections -mcpu=cortex-m3 -mthumb -T LPC1769_asm_basic_Debug.ld -o"LPC1769_asm_basic.axf" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '
	$(MAKE) --no-print-directory post-build

# Other Targets
clean:
	-$(RM) $(OBJS)$(C_DEPS)$(EXECUTABLES) LPC1769_asm_basic.axf
	-@echo ' '

post-build:
	-@echo 'Performing post-build steps'
	-arm-none-eabi-size LPC1769_asm_basic.axf; # arm-none-eabi-objcopy -O binary LPC1769_asm_basic.axf LPC1769_asm_basic.bin ; checksum -p LPC1769 -d LPC1769_asm_basic.bin;
	-@echo ' '

.PHONY: all clean dependents
.SECONDARY: post-build

-include ../makefile.targets

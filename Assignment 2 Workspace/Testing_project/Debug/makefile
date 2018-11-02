################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include src/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all: Testing_project.axf

# Tool invocations
Testing_project.axf: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: MCU Linker'
	arm-none-eabi-gcc -nostdlib -L"D:\Dropbox\Dropbox\NUS Modules\EE2028_EE2024\Assignments\Assignment 2\Assignment 2 Workspace\Lib_CMSISv1p30_LPC17xx\Debug" -L"D:\Dropbox\Dropbox\NUS Modules\EE2028_EE2024\Assignments\Assignment 2\Assignment 2 Workspace\Lib_EaBaseBoard\Debug" -L"D:\Dropbox\Dropbox\NUS Modules\EE2028_EE2024\Assignments\Assignment 2\Assignment 2 Workspace\Lib_MCU\Debug" -Xlinker --gc-sections -Xlinker -Map=Testing_project.map -mcpu=cortex-m3 -mthumb -T "Copy_of_demo_Debug.ld" -o "Testing_project.axf" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '
	$(MAKE) --no-print-directory post-build

# Other Targets
clean:
	-$(RM) $(OBJS)$(C_DEPS)$(EXECUTABLES) Testing_project.axf
	-@echo ' '

post-build:
	-@echo 'Performing post-build steps'
	-arm-none-eabi-size Testing_project.axf; # arm-none-eabi-objdump -h -S Testing_project.axf >Testing_project.lss
	-@echo ' '

.PHONY: all clean dependents
.SECONDARY: post-build

-include ../makefile.targets

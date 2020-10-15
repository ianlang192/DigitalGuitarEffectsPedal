################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/ADC.c \
../src/DAC.c \
../src/FLASH.c \
../src/LEDS.c \
../src/SPI1.c \
../src/SPI2.c \
../src/audio.c \
../src/clk_init.c \
../src/delay.c \
../src/display.c \
../src/effects.c \
../src/front_panel.c \
../src/main.c \
../src/stm32f4xx_it.c \
../src/syscalls.c \
../src/system_stm32f4xx.c 

OBJS += \
./src/ADC.o \
./src/DAC.o \
./src/FLASH.o \
./src/LEDS.o \
./src/SPI1.o \
./src/SPI2.o \
./src/audio.o \
./src/clk_init.o \
./src/delay.o \
./src/display.o \
./src/effects.o \
./src/front_panel.o \
./src/main.o \
./src/stm32f4xx_it.o \
./src/syscalls.o \
./src/system_stm32f4xx.o 

C_DEPS += \
./src/ADC.d \
./src/DAC.d \
./src/FLASH.d \
./src/LEDS.d \
./src/SPI1.d \
./src/SPI2.d \
./src/audio.d \
./src/clk_init.d \
./src/delay.d \
./src/display.d \
./src/effects.d \
./src/front_panel.d \
./src/main.d \
./src/stm32f4xx_it.d \
./src/syscalls.d \
./src/system_stm32f4xx.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32 -DSTM32F4 -DSTM32F446RCTx -DDEBUG -DSTM32F446xx -DUSE_HAL_DRIVER -I"C:/Users/Ian/Google Drive/School/Senior Project/Code/DGEP/HAL_Driver/Inc/Legacy" -I"C:/Users/Ian/Google Drive/School/Senior Project/Code/DGEP/inc" -I"C:/Users/Ian/Google Drive/School/Senior Project/Code/DGEP/CMSIS/device" -I"C:/Users/Ian/Google Drive/School/Senior Project/Code/DGEP/CMSIS/core" -I"C:/Users/Ian/Google Drive/School/Senior Project/Code/DGEP/HAL_Driver/Inc" -O3 -g3 -Wall -fmessage-length=0 -ffunction-sections -fdata-sections -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '



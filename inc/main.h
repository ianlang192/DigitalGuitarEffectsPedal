/*
 * main.h
 *
 *  Created on: Mar 7, 2018
 *      Author: Ian
 */

#ifndef MAIN_H_
#define MAIN_H_

#include "stm32f4xx_hal.h"
#include "stm32F4xx.h"
#include "SPI2.h"
#include "ADC.h"
#include "DAC.h"
#include "FLASH.h"
#include "delay.h"
#include "LEDS.h"
#include "clk_init.h"
#include "audio.h"
#include "front_panel.h"
#include "display.h"
#include "string.h"
#include "effects.h"

#define PERIPHERAL_CLK_FREQ_MHZ 	40
#define TIMER_CLK_FREQ_MHZ 			80
#define SYS_CLK_FREQ_MHZ			160


#define SYSTEM_STATE_DEFAULT		0
#define SYSTEM_STATE_ADJUST			1
#define SYSTEM_STATE_TUNER			2
#define SYSTEM_STATE_LOOPER			3
#define SYSTEM_STATE_TEMPO			4
#define SYSTEM_STATE_TEST			5

#define HEARTBEAT_LED_PERIOD_MS		500

void Initialize_Peripherals(void);
uint8_t Run_State_Default(void);
uint8_t Run_State_Adjust(void);
uint8_t Run_State_Tuner(void);
uint8_t Run_State_Looper(void);
uint8_t Run_State_Tempo(void);
uint8_t Run_State_Test(void);

#endif /* MAIN_H_ */

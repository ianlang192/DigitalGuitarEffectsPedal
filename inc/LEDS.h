/*
 * LEDS.h
 *
 *  Created on: Apr 5, 2018
 *      Author: Ian
 */

#ifndef LEDS_H_
#define LEDS_H_

#include "stm32F4xx.h"
#include "stm32f4xx_hal.h"

#define LED_POWER_PORT		GPIOC
#define LED_POWER_PIN		GPIO_PIN_13
#define LED_STATUS_PORT		GPIOB
#define LED_STATUS_PIN		GPIO_PIN_7
#define LED_HEARTBEAT_PORT	GPIOD
#define LED_HEARTBEAT_PIN	GPIO_PIN_2

#define OFF 				0
#define ON					1

void LEDS_Init(uint32_t timerFreqMHz);
void LED_Power(uint8_t level);
void LED_Status(uint8_t level);
void LED_Heartbeat(uint8_t level);
void LED_Blink_Heartbeat(uint16_t period_ms);
void LED_Blink_Status(uint16_t period_ms);

#endif /* LEDS_H_ */

/*
 * front_panel.h
 *
 *  Created on: Apr 28, 2018
 *      Author: Ian
 */

#ifndef FRONT_PANEL_H_
#define FRONT_PANEL_H_

#include "stm32F4xx.h"
#include "stm32f4xx_hal.h"

#define BUTTON_MENU_UP_Pos			8
#define BUTTON_MENU_UP_PIN			GPIO_PIN_8
#define BUTTON_MENU_UP_PORT			GPIOA
#define BUTTON_MENU_OK_Pos			10
#define BUTTON_MENU_OK_PIN			GPIO_PIN_10
#define BUTTON_MENU_OK_PORT			GPIOA
#define BUTTON_MENU_DOWN_Pos		0
#define BUTTON_MENU_DOWN_PIN		GPIO_PIN_0
#define BUTTON_MENU_DOWN_PORT		GPIOB
#define BUTTON_STOMP_UP_Pos			1
#define BUTTON_STOMP_UP_PIN			GPIO_PIN_1
#define BUTTON_STOMP_UP_PORT		GPIOB
#define BUTTON_STOMP_DOWN_Pos		5
#define BUTTON_STOMP_DOWN_PIN		GPIO_PIN_5
#define BUTTON_STOMP_DOWN_PORT		GPIOB
#define BUTTON_STOMP_EFFECT_Pos		6
#define BUTTON_STOMP_EFFECT_PIN		GPIO_PIN_6
#define BUTTON_STOMP_EFFECT_PORT	GPIOB

#define ENCODER_A_PIN	GPIO_PIN_8
#define ENCODER_B_PIN	GPIO_PIN_9
#define ENCODER_PORT	GPIOB

#define BUTTON_STATUS_MENU_UP_PRESS			1
#define BUTTON_STATUS_MENU_UP_HOLD			2
#define BUTTON_STATUS_MENU_OK_PRESS			3
#define BUTTON_STATUS_MENU_OK_HOLD			4
#define BUTTON_STATUS_MENU_DOWN_PRESS		5
#define BUTTON_STATUS_MENU_DOWN_HOLD		6
#define BUTTON_STATUS_STOMP_UP_PRESS		7
#define BUTTON_STATUS_STOMP_UP_HOLD			8
#define BUTTON_STATUS_STOMP_DOWN_PRESS		9
#define BUTTON_STATUS_STOMP_DOWN_HOLD		10
#define BUTTON_STATUS_STOMP_EFFECT_PRESS	11
#define BUTTON_STATUS_STOMP_EFFECT_HOLD		12

#define ENCODER_NO_ROTATION			0
#define ENCODER_COUNTERCLOCKWISE	1
#define ENCODER_CLOCKWISE 			2

#define ENCODER_TICK_CNT		2 		// Amount of encoder ticks required to register as rotating
#define BUTTON_LONG_PRESS_MS 	1000
#define BUTTON_LOCKOUT_MS		50		// Lockout time for buttons

void Buttons_Init(uint32_t timerFreqMHz);
void Encoder_Init(uint32_t timerFreqMHz);
uint8_t Front_Panel_Encoder_Rotation(void);
void Front_Panel_Reset_Encoder_Rotation(void);
uint8_t Front_Panel_Button_Status(void);
void Reset_Front_Panel_Button_Status();
uint8_t Front_Panel_Event(void);
void Clear_Front_Panel_Event(void);

#endif /* FRONT_PANEL_H_ */

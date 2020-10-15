/*
 * DISPLAY.h
 *
 *  Created on: Apr 3, 2018
 *      Author: Ian
 */

#ifndef DISPLAY_H_
#define DISPLAY_H_

#include "stm32F4xx.h"
#include "stm32f4xx_hal.h"
#include "delay.h"
#include "string.h"

#define GPIOx				GPIOC
#define DISPLAY_DB0_PIN 	GPIO_PIN_0
#define DISPLAY_DB1_PIN 	GPIO_PIN_1
#define DISPLAY_DB2_PIN 	GPIO_PIN_2
#define DISPLAY_DB3_PIN 	GPIO_PIN_3
#define DISPLAY_DB4_PIN 	GPIO_PIN_4
#define DISPLAY_DB5_PIN 	GPIO_PIN_5
#define DISPLAY_DB6_PIN 	GPIO_PIN_6
#define DISPLAY_DB7_PIN 	GPIO_PIN_7
#define DISPLAY_RS_PIN 		GPIO_PIN_8
#define DISPLAY_RW_PIN 		GPIO_PIN_9
#define DISPLAY_E_PIN 		GPIO_PIN_10

#define DISPLAY_DB_Msk 		0xFF
#define DISPLAY_DB_Pos		0

#define DISPLAY_INST_DELAY_US	40
#define DISPLAY_CLEAR_DELAY_US 	1600
#define DISPLAY_E_DELAY_NS		200

#define DISPLAY_INST_CLEAR_DISPLAY		0x01			// Clear display instruction

#define HIGH				1
#define LOW					0
#define DISPLAY_ALIGN_RIGHT 0
#define DISPLAY_ALIGN_LEFT 	1

void Display_Init(uint32_t timerFreqMHz);
void Display_Command(uint8_t command);
void Display_Write(uint8_t* pData, uint8_t numChars, uint8_t startingPos);
void Display_Write_Number(uint16_t number, uint8_t digits, uint8_t startingPos, uint8_t alignment);
void Display_Write_String(char* pData, uint8_t startingPos);
void Display_Write_Char(char data, uint8_t startingPos);
void Display_Erase_Area(uint8_t startingPos, uint8_t length);
void Display_Write_Bar_Graph(uint8_t startingPos, uint8_t length, uint16_t value, uint16_t minValue, uint16_t maxValue);
void Display_Clear(void);

#endif /* DISPLAY_H_ */

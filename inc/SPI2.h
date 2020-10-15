/*
 * SPI2.h
 *
 *  Created on: Mar 14, 2018
 *      Author: Ian
 */

#ifndef SPI2_H_
#define SPI2_H_

#include "stm32F4xx.h"
#include "stm32f4xx_hal.h"

#define SPI2_PORT 			GPIOB
#define SPI2_SCK_PIN 		GPIO_PIN_13
#define SPI2_MOSI_PIN 		GPIO_PIN_15
#define SPI2_MISO_PIN 		GPIO_PIN_14


void SPI2_Init(void);
void SPI2_Read_Write(uint8_t* pData, uint16_t numBytes);
void SPI2_Disable(void);

#endif /* SPI2_H_ */

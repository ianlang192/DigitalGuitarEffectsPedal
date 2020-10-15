/*
 * SPI1.h
 *
 *  Created on: Apr 3, 2018
 *      Author: Ian
 */

#ifndef SPI1_H_
#define SPI1_H_

#include "stm32F4xx.h"
#include "stm32f4xx_hal.h"

#define SPI1_PORT 			GPIOA
#define SPI1_SCK_PIN 		GPIO_PIN_5
#define SPI1_MOSI_PIN 		GPIO_PIN_7
#define SPI1_MISO_PIN 		GPIO_PIN_6


void SPI1_Init(void);
void SPI1_Read_Write(uint8_t* pRxData, uint8_t* pTxData, uint16_t numBytes);
void SPI1_Disable(void);
void SPI1_Set_Complete_Flag(void);
uint8_t SPI1_Get_Complete_Flag(void);
void SPI1_Reset_Complete_Flag(void);

#endif /* SPI1_H_ */

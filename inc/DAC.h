/*
 * DAC.h
 *
 *  Created on: Mar 15, 2018
 *      Author: Ian
 */

#ifndef DAC_H_
#define DAC_H_

#include "stm32F4xx.h"
#include "stm32f4xx_hal.h"
#include "SPI2.h"

#define SPI2_CSn_DAC_PIN	GPIO_PIN_12
#define SPI2_CSn_DAC_PORT	GPIOB

void DAC_Init(void);
void DAC_Write(uint16_t* data);
void DAC_Set_Complete_Flag(void);
uint8_t DAC_Get_Complete_Flag(void);
void DAC_Reset_Complete_Flag(void);
#endif /* DAC_H_ */

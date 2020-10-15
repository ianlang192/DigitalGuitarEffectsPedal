/*
 * ADC.h
 *
 *  Created on: Mar 14, 2018
 *      Author: Ian
 */

#ifndef ADC_H_
#define ADC_H_

#include "stm32F4xx.h"
#include "stm32f4xx_hal.h"
#include "SPI2.h"

#define SPI2_CSn_ADC_PIN	GPIO_PIN_10
#define SPI2_CSn_ADC_PORT	GPIOB

#define T_CONV_NS 1500
#define TIMER_PRESCALE 0
void ADC_Init(uint32_t timerFreqMHz);
void ADC_Start_Conversion(void);
void ADC_Start_Acquisition(void);
uint16_t ADC_Get_Result(void);
void ADC_Set_Complete_Flag(void);
void ADC_Reset_Complete_Flag(void);
uint8_t ADC_Get_Complete_Flag(void);



#endif /* ADC_H_ */

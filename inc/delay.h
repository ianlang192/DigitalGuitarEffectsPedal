/*
 * delay.h
 *
 *  Created on: Apr 3, 2018
 *      Author: Ian
 */

#ifndef DELAY_H_
#define DELAY_H_

#include "stm32F4xx.h"
#include "stm32f4xx_hal.h"

void Delay_Init(uint32_t timerFreqMHz);
void Delay_us(uint16_t delayMicroseconds);
void Delay_ns(uint16_t delayNanoseconds);

#endif /* DELAY_H_ */

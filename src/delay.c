/*
 * delay.c
 *
 *  Created on: Apr 3, 2018
 *      Author: Ian
 */

#include "delay.h"

static uint32_t TIMER_FREQ_MHZ;

void Delay_Init(uint32_t timerFreqMHz)
{
	TIMER_FREQ_MHZ = timerFreqMHz;

	// Set up TIM10
	TIM10->CR1 |= (	TIM_CR1_ARPE 	|		// Enable auto-preload
					TIM_CR1_URS);			// Only overflow generates interrupt
	TIM10->EGR |= TIM_EGR_UG;				// Generate update of registers
	TIM10->SR &= ~TIM_SR_UIF; 				// Clear update flag
}


void Delay_ns(uint16_t delayNanoseconds)
{
	uint32_t timerCount = TIMER_FREQ_MHZ * delayNanoseconds / 1000 + 1;
	TIM10->ARR = (uint16_t)timerCount;
	TIM10->PSC = 0;							// Clear prescaler
	TIM10->EGR |= TIM_EGR_UG;				// Generate update of registers
	TIM10->SR &= ~TIM_SR_UIF;				// Reset update flag
	TIM10->CR1 |= TIM_CR1_CEN; 				// Start the timer
	while((TIM10->SR & TIM_SR_UIF) == 0){}	// Wait for timer to overflow
	TIM10->CR1 &= ~TIM_CR1_CEN; 			// Stop the timer
}

/*
 * Delay a number of microseconds
 */
void Delay_us(uint16_t delayMicroseconds)
{
	TIM10->ARR = delayMicroseconds;
	TIM10->PSC = TIMER_FREQ_MHZ;			// Prescale the timer to give 1us per timer count
	TIM10->EGR |= TIM_EGR_UG;				// Generate update of registers
	TIM10->SR &= ~TIM_SR_UIF;				// Reset update flag
	TIM10->CR1 |= TIM_CR1_CEN; 				// Start the timer
	while((TIM10->SR & TIM_SR_UIF) == 0){}	// Wait for timer to overflow
	TIM10->CR1 &= ~TIM_CR1_CEN; 			// Stop the timer
}

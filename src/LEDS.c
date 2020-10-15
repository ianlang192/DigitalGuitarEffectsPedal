/*
 * LEDS.c
 *
 *  Created on: Apr 5, 2018
 *      Author: Ian
 */

#include "LEDS.h"
#define TIM_PRESCALE 40000

static uint32_t TIMER_FREQ_MHZ;

static volatile uint8_t LEDPowerLevel = 0;
static volatile uint8_t LEDStatusLevel = 0;
static volatile uint8_t LEDHeartbeatLevel = 0;

void LEDS_Init(uint32_t timerFreqMHz)
{
	TIMER_FREQ_MHZ = timerFreqMHz;

	GPIO_InitTypeDef GPIO_InitStucture;

	GPIO_InitStucture.Mode = 		GPIO_MODE_OUTPUT_PP;		// Output, push pull mode
	GPIO_InitStucture.Speed =		GPIO_SPEED_FREQ_MEDIUM ;	// Set speed for 25Mhz-50MHz
	GPIO_InitStucture.Pull = 		GPIO_NOPULL;				// No pullups or pulldowns

	LED_Power(ON);
	LED_Status(OFF);
	LED_Heartbeat(OFF);

	GPIO_InitStucture.Pin = 		LED_POWER_PIN;				// Select power LED
	HAL_GPIO_Init(LED_POWER_PORT, &GPIO_InitStucture);			// Initialize
	GPIO_InitStucture.Pin = 		LED_STATUS_PIN;				// Select status LED
	HAL_GPIO_Init(LED_STATUS_PORT, &GPIO_InitStucture);			// Initialize
	GPIO_InitStucture.Pin = 		LED_HEARTBEAT_PIN;			// Select heartbeat LED
	HAL_GPIO_Init(LED_HEARTBEAT_PORT, &GPIO_InitStucture);		// Initialize

	TIM6->CR1 |= 	TIM_CR1_URS;			// Only generate update on overflow
	TIM6->DIER |=	TIM_DIER_UIE;			// Enable update interrupt
	TIM6->PSC = 	TIM_PRESCALE;			// Set timer prescale amount
	NVIC_SetPriority(TIM6_DAC_IRQn, 10);
	NVIC_EnableIRQ(TIM6_DAC_IRQn);

	TIM7->CR1 |= 	TIM_CR1_URS;			// Only generate update on overflow
	TIM7->DIER |=	TIM_DIER_UIE;			// Enable update interrupt
	TIM7->PSC = 	TIM_PRESCALE;			// Set timer prescale amount
	NVIC_SetPriority(TIM7_IRQn, 10);
	NVIC_EnableIRQ(TIM7_IRQn);
}


void LED_Power(uint8_t level)
{
	if(level)
	{
		LED_POWER_PORT->BSRR = LED_POWER_PIN;
		LEDPowerLevel = 1;
	}
	else
	{
		LED_POWER_PORT->BSRR = (LED_POWER_PIN << GPIO_BSRR_BR0_Pos);
		LEDPowerLevel = 0;
	}
}

void LED_Status(uint8_t level)
{
	if(level)
	{
		LED_STATUS_PORT->BSRR = LED_STATUS_PIN;
		LEDStatusLevel = 1;
	}
	else
	{
		LED_STATUS_PORT->BSRR = (LED_STATUS_PIN << GPIO_BSRR_BR0_Pos);
		LEDStatusLevel = 0;
	}
}

void LED_Heartbeat(uint8_t level)
{
	if(level)
	{
		LED_HEARTBEAT_PORT->BSRR = LED_HEARTBEAT_PIN;
		LEDHeartbeatLevel = 1;
	}
	else
	{
		LED_HEARTBEAT_PORT->BSRR = (LED_HEARTBEAT_PIN << GPIO_BSRR_BR0_Pos);
		LEDHeartbeatLevel = 0;
	}
}

void LED_Blink_Heartbeat(uint16_t period_ms)
{
	if(period_ms == 0)
	{
		TIM6->CR1 &= ~TIM_CR1_CEN; // Disable the timer
		LED_Heartbeat(OFF);
		return;
	}
	TIM6->ARR = (TIMER_FREQ_MHZ * period_ms * 1000) / (TIM_PRESCALE * 2);
	TIM6->EGR |= TIM_EGR_UG;	// Generate update of registers
	TIM6->SR &= ~TIM_SR_UIF; 	// Clear update flag
	TIM6->CR1 |= TIM_CR1_CEN;	// Enable the timer
}

void LED_Blink_Status(uint16_t period_ms)
{
	if(period_ms == 0)
	{
		TIM7->CR1 &= ~TIM_CR1_CEN; // Disable the timer
		LED_Status(OFF);
		return;
	}
	TIM7->ARR = (TIMER_FREQ_MHZ * period_ms * 1000) / (TIM_PRESCALE * 2);
	TIM7->EGR |= TIM_EGR_UG;	// Generate update of registers
	TIM7->SR &= ~TIM_SR_UIF; 	// Clear update flag
	TIM7->CR1 |= TIM_CR1_CEN;	// Enable the timer
}

void TIM6_DAC_IRQHandler(void)
{
	if(TIM6->SR & TIM_SR_UIF)
	{
		TIM6->SR &= ~TIM_SR_UIF; // Reset flag
		if(LEDHeartbeatLevel)
			LED_Heartbeat(OFF);
		else
			LED_Heartbeat(ON);
	}
}

void TIM7_IRQHandler(void)
{
	if(TIM7->SR & TIM_SR_UIF)
	{
		TIM7->SR &= ~TIM_SR_UIF; // Reset flag
		if(LEDStatusLevel)
			LED_Status(OFF);
		else
			LED_Status(ON);
	}
}


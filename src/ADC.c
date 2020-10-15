/*
 * ADC.c
 *
 *  Created on: Mar 14, 2018
 *      Author: Ian Lang
 *
 *      Configures the ADS8319 ADC on SPI2. The ADC operates in "3 wire CS mode
 *      without busy indicator." A rising edge on CSn starts the conversion, then
 *      a falling edge of CSn starts the data transfer after the conversion time
 *      has elapsed. CSn is left low until the next conversion is to begin.
 *
 *      Max conversion time is 1.4us
 *
 *      Uses timer 9 to track the conversion time
 */

#include "ADC.h"

static void ADC_Set_CS_High(void);
static void ADC_Set_CS_Low(void);

static uint32_t TIMER_FREQ_MHZ;
volatile static uint8_t ADCResult[2];
volatile static uint8_t ADCCompleteFlag = 0;
volatile static uint8_t busyFlag = 0;

void ADC_Init(uint32_t timerFreqMHz)
{
	TIMER_FREQ_MHZ = timerFreqMHz;

	GPIO_InitTypeDef GPIO_InitStucture;
	GPIO_InitStucture.Pin = 		SPI2_CSn_ADC_PIN;		// Select chip select pins
	GPIO_InitStucture.Mode = 		GPIO_MODE_OUTPUT_PP;	// Output, push pull mode
	GPIO_InitStucture.Speed =		GPIO_SPEED_FREQ_VERY_HIGH ;	// Set speed for 25Mhz-50MHz
	GPIO_InitStucture.Pull = 		GPIO_NOPULL;			// No pullups or pulldowns
	ADC_Set_CS_Low();										// Set CS pin to low by default
	HAL_GPIO_Init(SPI2_CSn_ADC_PORT, &GPIO_InitStucture);			// Initialize

	// Set up TIM9
	TIM9->CR1 |= (	TIM_CR1_ARPE 	|		// Enable auto-preload
					TIM_CR1_OPM 	|		// Set one pulse mode
					TIM_CR1_URS);			// Only overflow generates interrupt
	TIM9->DIER |= 	TIM_DIER_UIE;			// Generate interrupt on overflow
	TIM9->PSC = 	TIMER_PRESCALE;			// Prescale the timer
	uint32_t timerCount = TIMER_FREQ_MHZ * T_CONV_NS / 1000;
	TIM9->ARR = (uint16_t)timerCount;	// Calculate required timer value
	TIM9->EGR |= TIM_EGR_UG;			// Generate update of registers
	TIM9->SR &= ~TIM_SR_UIF; 			// Clear update flag

	NVIC_SetPriority(TIM1_BRK_TIM9_IRQn,4); // Set interrupt priority
	NVIC_EnableIRQ(TIM1_BRK_TIM9_IRQn);		// Enable overflow interrupt
}

static void ADC_Set_CS_High(void)
{
	SPI2_CSn_ADC_PORT->BSRR =  	SPI2_CSn_ADC_PIN;
}
static void ADC_Set_CS_Low(void)
{
	SPI2_CSn_ADC_PORT->BSRR =  	(SPI2_CSn_ADC_PIN << GPIO_BSRR_BR0_Pos);
}

void ADC_Start_Conversion(void)
{
	busyFlag = 1;				// Set busy flag to signal transmission in progress
	ADC_Set_CS_High();			// Set CS high to start conversion
	TIM9->CR1 |= TIM_CR1_CEN; 	// Start the timer
}

void ADC_Start_Acquisition(void)
{
	ADC_Set_CS_Low(); 					// Set CS low to start acquisition
	SPI2_Read_Write(&ADCResult[0], 2);	// Start the SPI transmission
}

void ADC_Set_Complete_Flag(void)
{
	if(busyFlag)
	{
		EXTI->SWIER |= EXTI_SWIER_SWIER2; // Generate EXTI2 interrupt
		ADCCompleteFlag = 1;
		busyFlag = 0;
	}
}
void ADC_Reset_Complete_Flag(void)
{
	ADCCompleteFlag = 0;
}

uint8_t ADC_Get_Complete_Flag(void)
{
	return ADCCompleteFlag;
}
uint16_t ADC_Get_Result(void)
{
	return ((uint16_t)ADCResult[0] << 8) | (uint16_t)ADCResult[1]; // Combine the two bytes and return
}

void TIM1_BRK_TIM9_IRQHandler(void)
{
	if(TIM9->SR & TIM_SR_UIF) // If update flag is set
	{
		TIM9->SR &= ~TIM_SR_UIF; // Clear interrupt flag
		ADC_Start_Acquisition(); // Start the data transfer
	}
}

/*
 * DAC.c
 *
 *  Created on: Mar 15, 2018
 *      Author: Ian
 */

#include "DAC.h"

static void DAC_Set_CS_High(void);
static void DAC_Set_CS_Low(void);

volatile static uint8_t DACCompleteFlag = 0;
volatile static uint8_t dataBuf[3];
volatile static uint8_t busyFlag = 0;

void DAC_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStucture;
	GPIO_InitStucture.Pin = 		SPI2_CSn_DAC_PIN;		// Select chip select pins
	GPIO_InitStucture.Mode = 		GPIO_MODE_OUTPUT_PP;	// Output, push pull mode
	GPIO_InitStucture.Speed =		GPIO_SPEED_FREQ_HIGH;	// Set speed for 25Mhz-50MHz
	GPIO_InitStucture.Pull = 		GPIO_NOPULL;			// No pullups or pulldowns
	DAC_Set_CS_High();										// Set CS pin to high by default
	HAL_GPIO_Init(SPI2_CSn_DAC_PORT, &GPIO_InitStucture);	// Initialize
}

static void DAC_Set_CS_High(void)
{
	SPI2_CSn_DAC_PORT->BSRR =  	SPI2_CSn_DAC_PIN;
}
static void DAC_Set_CS_Low(void)
{
	SPI2_CSn_DAC_PORT->BSRR =  	(SPI2_CSn_DAC_PIN << GPIO_BSRR_BR0_Pos);
}
void DAC_Write(uint16_t* data)
{
	busyFlag = 1;						// Indicate transmission in progress
	DAC_Set_CS_Low();					// Set CS low
	dataBuf[0] = 0x00; 					// First byte sets normal operation
	dataBuf[1] = (*data & 0xFF00) >> 8;
	dataBuf[2] = (*data & 0x00FF);
	SPI2_Read_Write(&dataBuf[0],3);
}

void DAC_Set_Complete_Flag(void)
{
	if(busyFlag)
	{
		DAC_Set_CS_High(); // Bring CS high after transmission is complete
		DACCompleteFlag = 1;
		busyFlag = 0;
	}
}
uint8_t DAC_Get_Complete_Flag(void)
{
	return DACCompleteFlag;
}
void DAC_Reset_Complete_Flag(void)
{
	DACCompleteFlag = 0;
}

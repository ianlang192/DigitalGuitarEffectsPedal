/*
 * SPI1.c
 *
 *  Created on: Apr 3, 2018
 *      Author: Ian
 */

#include "SPI1.h"

volatile static uint8_t completeFlag = 0;

/* SPI1 init function */
void SPI1_Init(void)
{
	// Initialize SPI2 GPIO
	GPIO_InitTypeDef GPIO_InitStucture;
	GPIO_InitStucture.Pin = (		SPI1_SCK_PIN  |			// Select SPI1 pins
									SPI1_MOSI_PIN |
									SPI1_MISO_PIN);
	GPIO_InitStucture.Mode = 		GPIO_MODE_AF_PP;		// Alternate function, push pull mode
	GPIO_InitStucture.Speed = 		GPIO_SPEED_FREQ_VERY_HIGH;	// Set speed for 50Mhz-200MHz
	GPIO_InitStucture.Pull = 		GPIO_NOPULL;			// No pullups or pulldowns
	GPIO_InitStucture.Alternate = 	GPIO_AF5_SPI1;			// Set alternate function for SPI1 pins
	HAL_GPIO_Init(SPI1_PORT, &GPIO_InitStucture);			// Initialize


	SPI_HandleTypeDef hspi1;
	/* SPI2 parameter configuration*/
	hspi1.Instance = SPI1;
	hspi1.Init.Mode = SPI_MODE_MASTER;
	hspi1.Init.Direction = SPI_DIRECTION_2LINES;
	hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
	hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
	hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
	hspi1.Init.NSS = SPI_NSS_SOFT;
	hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
	hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
	hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
	hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
	hspi1.Init.CRCPolynomial = 10;
	HAL_SPI_Init(&hspi1);



	// Initialize RX DMA access (DMA2 Stream 2 CH3)
	DMA2_Stream2->PAR = &(SPI1->DR);			// Point the peripheral address to the data register
	DMA2_Stream2->NDTR = 0;						// Initialize received bytes to 0
	DMA2_Stream2->CR |= ( 	DMA_SxCR_CHSEL_0 | 	// Channel 3
							DMA_SxCR_CHSEL_1 |
							DMA_SxCR_PL_1 	|	// Set priority high
							DMA_SxCR_MINC 	|	// Increment memory address
							DMA_SxCR_TCIE); 	// Enable transfer complete interrupt
	NVIC_SetPriority(DMA2_Stream2_IRQn,2); 		// Set priority of DMA interrupt
	NVIC_EnableIRQ(DMA2_Stream2_IRQn);			// Enable DMA interrupt

	// Initialize TX DMA access (DMA2 Stream 3 CH3)
	DMA2_Stream3->PAR = &(SPI1->DR);
	DMA2_Stream3->NDTR = 0; 					// Initialize transmitted bytes to 0
	DMA2_Stream3->CR |= ( 	DMA_SxCR_CHSEL_0 |  // Channel 3
							DMA_SxCR_CHSEL_1 |
							DMA_SxCR_PL_1 |		// Set priority high
							DMA_SxCR_MINC |		// Increment memory address
							DMA_SxCR_TCIE |		// Enable transfer complete interrupt
							DMA_SxCR_DIR_0);	// Set memory to peripheral direction
	NVIC_SetPriority(DMA2_Stream3_IRQn,2); 		// Set priority of DMA interrupt
	NVIC_EnableIRQ(DMA2_Stream3_IRQn);			// Enable DMA interrupt
}


void SPI1_Read_Write(uint8_t* pRxData, uint8_t* pTxData, uint16_t numBytes)
{
	// Enable SPI with DMA, order is as recommended in reference manual
	SPI1->CR2 |= SPI_CR2_RXDMAEN;		// DMA request enable on RXNE flag
	DMA2_Stream2->M0AR = pRxData;		// Set the memory address for RX
	DMA2_Stream2->NDTR = numBytes; 		// Set number of received bytes
	DMA2_Stream2->CR |= DMA_SxCR_EN; 	// Enable DMA RX stream
	DMA2_Stream3->M0AR = pTxData;		// Set the memory address for TX
	DMA2_Stream3->NDTR = numBytes; 		// Set number of transmitted bytes
	DMA2_Stream3->CR |= DMA_SxCR_EN; 	// Enable DMA TX stream
	SPI1->CR2 |= SPI_CR2_TXDMAEN;		// DMA request enable on TXE flag
	SPI1->CR1 |= SPI_CR1_SPE;			// Enable SPI1
}

void SPI1_Disable(void)
{
	DMA2_Stream2->CR &= ~DMA_SxCR_EN; 	// Disable DMA stream
	DMA2_Stream3->CR &= ~DMA_SxCR_EN; 	// Disable DMA stream
	SPI1->CR1 &= ~SPI_CR1_SPE;			// Disable SPI1
	SPI1->CR2 &= ~(	SPI_CR2_RXDMAEN |	// Disable DMA TX and RX buffers
					SPI_CR2_TXDMAEN);
}

void SPI1_Set_Complete_Flag(void)
{
	completeFlag = 1;
}
uint8_t SPI1_Get_Complete_Flag(void)
{
	return completeFlag;
}
void SPI1_Reset_Complete_Flag(void)
{
	completeFlag = 0;
}


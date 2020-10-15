/*
 * SPI2.c
 *
 *  Created on: Mar 14, 2018
 *      Author: Ian
 */

#include "SPI2.h"
#include "DAC.h"
/* SPI2 init function */
void SPI2_Init(void)
{
	// Initialize SPI2 GPIO
	GPIO_InitTypeDef GPIO_InitStucture;
	GPIO_InitStucture.Pin = (		SPI2_SCK_PIN  |			// Select SPI2 pins
									SPI2_MOSI_PIN |
									SPI2_MISO_PIN);
	GPIO_InitStucture.Mode = 		GPIO_MODE_AF_PP;		// Alternate function, push pull mode
	GPIO_InitStucture.Speed = 		GPIO_SPEED_FREQ_VERY_HIGH;	// Set speed for 50Mhz-200MHz
	GPIO_InitStucture.Pull = 		GPIO_NOPULL;			// No pullups or pulldowns
	GPIO_InitStucture.Alternate = 	GPIO_AF5_SPI2;			// Set alternate function for SPI2 pins
	HAL_GPIO_Init(SPI2_PORT, &GPIO_InitStucture);			// Initialize


	SPI_HandleTypeDef hspi2;
	/* SPI2 parameter configuration*/
	hspi2.Instance = SPI2;
	hspi2.Init.Mode = SPI_MODE_MASTER;
	hspi2.Init.Direction = SPI_DIRECTION_2LINES;
	hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
	hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
	hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
	hspi2.Init.NSS = SPI_NSS_SOFT;
	hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
	hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
	hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
	hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
	hspi2.Init.CRCPolynomial = 10;
	if (HAL_SPI_Init(&hspi2) != HAL_OK)
	{
		//_Error_Handler(__FILE__, __LINE__);
	}

	// Initialize RX DMA access (DMA1 Stream 3 CH0)
	DMA1_Stream3->PAR = &(SPI2->DR);		// Point the peripheral address to the data register
	DMA1_Stream3->NDTR = 0;					// Reset number of bytes to send
	DMA1_Stream3->CR |= ( 	DMA_SxCR_PL_1 |	// Set priority high
							DMA_SxCR_MINC |	// Increment memory address
							DMA_SxCR_TCIE); // Enable transfer complete interrupt
	NVIC_SetPriority(DMA1_Stream3_IRQn,1); 	// Set priority of DMA interrupt
	NVIC_EnableIRQ(DMA1_Stream3_IRQn);		// Enable DMA interrupt

	// Initialize TX DMA access (DMA1 Stream 4 CH0)
	DMA1_Stream4->PAR = &(SPI2->DR);
	DMA1_Stream4->NDTR = 0;						// Reset number of bytes to send
	DMA1_Stream4->CR |= ( 	DMA_SxCR_PL_1 |		// Set priority high
							DMA_SxCR_MINC |		// Increment memory address
							DMA_SxCR_DIR_0);	// Set memory to peripheral direction
}

void SPI2_Read_Write(uint8_t* pData, uint16_t numBytes)
{
	// Enable SPI with DMA, order is as recommended in reference manual
	SPI2->CR2 |= SPI_CR2_RXDMAEN;		// DMA request enable on RXNE flag
	DMA1_Stream3->M0AR = pData;			// Set the memory address for RX
	DMA1_Stream3->NDTR = numBytes; 		// Set number of received bytes
	DMA1_Stream3->CR |= DMA_SxCR_EN; 	// Enable DMA RX stream
	DMA1_Stream4->M0AR = pData;			// Set the memory address for TX
	DMA1_Stream4->NDTR = numBytes; 		// Set number of transmitted bytes
	DMA1_Stream4->CR |= DMA_SxCR_EN; 	// Enable DMA TX stream
	SPI2->CR2 |= SPI_CR2_TXDMAEN;		// DMA request enable on TXE flag
	SPI2->CR1 |= SPI_CR1_SPE;			// Enable SPI2
}

void SPI2_Disable(void)
{
	DMA1_Stream3->CR &= ~DMA_SxCR_EN; 	// Disable DMA stream
	DMA1_Stream4->CR &= ~DMA_SxCR_EN; 	// Disable DMA stream
	DMA1->HIFCR |= DMA_HIFCR_CTCIF4;	// Clear transmit complete flag from stream 4 (Does not work without this)
	SPI2->CR1 &= ~SPI_CR1_SPE;			// Disable SPI2
	SPI2->CR2 &= ~(	SPI_CR2_RXDMAEN |	// Disable DMA TX and RX buffers
					SPI_CR2_TXDMAEN);
}

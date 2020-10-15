/*
 * DISPLAY.c
 *
 *  Created on: Apr 3, 2018
 *      Author: Ian
 */

#include <display.h>

typedef struct
{
	uint8_t data[20];
	uint8_t numChars;
	uint8_t command;
	uint16_t delay_us;

} Display_Data_TypeDef;

#define TIM11_PRESCALE 	4

static void Display_Set_E(uint8_t value);
static void Display_Write_IT(uint8_t* pData, uint8_t numChars, uint8_t command);
static void Display_Fast_Write_IT(void);

static uint8_t spaces[] = {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '};
static uint8_t bars[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

static uint32_t TIMER_FREQ_MHZ;
static uint8_t* pDataBuf; // Pointer to character data buffer
static uint8_t charIndex;
static volatile uint8_t busyFlag = 0;
static Display_Data_TypeDef FIFO_buffer[32];
static volatile uint8_t FIFO_tail = 0;
static volatile uint8_t FIFO_head = 0;


void Display_Init(uint32_t timerFreqMHz)
{
	TIMER_FREQ_MHZ = timerFreqMHz;

	GPIO_InitTypeDef GPIO_InitStucture;
	GPIO_InitStucture.Pin = (		DISPLAY_DB0_PIN 	|		// Select display control pins
									DISPLAY_DB1_PIN		|
									DISPLAY_DB2_PIN 	|
									DISPLAY_DB3_PIN 	|
									DISPLAY_DB4_PIN 	|
									DISPLAY_DB5_PIN 	|
									DISPLAY_DB6_PIN 	|
									DISPLAY_DB7_PIN 	|
									DISPLAY_RS_PIN 		|
									DISPLAY_RW_PIN 		|
									DISPLAY_E_PIN 		);
	GPIO_InitStucture.Mode = 		GPIO_MODE_OUTPUT_PP;	// Output, push pull mode
	GPIO_InitStucture.Speed =		GPIO_SPEED_FREQ_VERY_HIGH ;	// Set speed
	GPIO_InitStucture.Pull = 		GPIO_NOPULL;			// No pullups or pulldowns
	Display_Set_E(LOW);										// Set enable low by default
	HAL_GPIO_Init(GPIOx, &GPIO_InitStucture);				// Initialize GPIO

	// Set up TIM11
	TIM11->CR1 |= (	TIM_CR1_ARPE 	|		// Enable auto-preload
					TIM_CR1_URS);			// Only overflow generates interrupt
	TIM11->DIER |= (TIM_DIER_CC1IE	|		// Enable CCR1 interrupt
					TIM_DIER_UIE);			// Enable overflow interrupt
	TIM11->PSC = 	TIM11_PRESCALE;
	TIM11->CCR1 =	(uint16_t)(TIMER_FREQ_MHZ * DISPLAY_E_DELAY_NS / (1000 * (TIM11_PRESCALE + 1)));
	TIM11->EGR |= TIM_EGR_UG;				// Generate update of registers
	TIM11->SR &= ~TIM_SR_UIF; 				// Clear update flag
	NVIC_SetPriority(TIM1_TRG_COM_TIM11_IRQn,6); 	// Set interrupt priority
	NVIC_EnableIRQ(TIM1_TRG_COM_TIM11_IRQn);		// Enable timer interrupt

	// Initialize DMA access (DMA2 Stream 0 CH1)
	DMA2_Stream0->CR |= ( 	DMA_SxCR_CHSEL_0 	| 	// Channel 1
							DMA_SxCR_PL_1 		|	// Set priority high
							DMA_SxCR_MINC 		|	// Increment memory address
							DMA_SxCR_PINC 		|	// Increment peripheral address
							DMA_SxCR_DIR_1		|	// Memory-to-memory direction
							DMA_SxCR_TCIE);			// Transfer complete interrupt enable

	Delay_us(64000); 							// Wait 40ms after power on
	Delay_us(64000); 							// Wait 40ms after power on
	Display_Command(0x3C); 						// Wake up display
	Display_Command(0x3C); 						// Wake up display
	Display_Command(0x3C); 						// Wake up display
	Display_Command(0x0C);						// Turn on display
	Display_Clear();							// Clear Display
	Display_Command(0x06); 						// Entry mode

	uint8_t arrowDown[] = { 0x00, 0x00, 0x00, 0x1F, 0x0E, 0x04, 0x00, 0x00};
	Display_Write_IT(&arrowDown[0], 8, 0x40); // Set CGRAM address 0
	uint8_t arrowUp[] = { 0x00, 0x00, 0x04, 0x0E, 0x1F, 0x00, 0x00, 0x00};
	Display_Write_IT(&arrowUp[0], 8, 0x48); // Set CGRAM address 1
}

void Display_Command(uint8_t command)
{
	uint8_t tempData = 0;							// Placeholder since no data is written
	Display_Write_IT(&tempData, 0, command);
}

void Display_Write(uint8_t* pData, uint8_t numChars, uint8_t startingPos)
{
	Display_Write_IT(pData, numChars, (0x80 | startingPos));
}

void Display_Write_Number(uint16_t number, uint8_t digits, uint8_t startingPos, uint8_t alignment)
{
	uint8_t buffer[digits];
	int8_t i;
	if(alignment == DISPLAY_ALIGN_RIGHT)
	{
		startingPos -= (digits - 1);
		for(i = (digits - 1); i >= 0; i--)
		{
			if(number == 0 && i != (digits - 1))
			{
				buffer[i] = 0x10;			// Enter blank space
			}
			else
				buffer[i] = 0x30 | (number % 10);
			number = number / 10;
		}
		Display_Write_IT(&buffer[0], digits, (0x80 | startingPos));
	}
	else if(alignment == DISPLAY_ALIGN_LEFT)
	{
		uint8_t startingLoc = 0;
		for(i = (digits - 1); i >= 0; i--)
		{
			if(number == 0 && i != (digits - 1))
			{
				startingLoc = i+1;
				digits -= (1 + i);
				break;
			}
			else
				buffer[i] = 0x30 | (number % 10);
			number = number / 10;
		}
		Display_Write_IT(&buffer[startingLoc], digits, (0x80 | startingPos));
	}
}

void Display_Write_String(char* pData, uint8_t startingPos)
{
	Display_Write_IT((uint8_t*)pData, strlen(pData), (0x80 |startingPos));
}

void Display_Write_Char(char data, uint8_t startingPos)
{
	Display_Write_IT(&data, 1, (0x80 |startingPos));
}

void Display_Erase_Area(uint8_t startingPos, uint8_t length)
{
	if(length > 20)
		length = 20;
	Display_Write_IT(&spaces[0], length, (0x80 | startingPos));
}

void Display_Write_Bar_Graph(uint8_t startingPos, uint8_t length, uint16_t value, uint16_t minValue, uint16_t maxValue)
{
	Display_Erase_Area(startingPos, length);
	uint8_t barLength = value * length / (maxValue - minValue);
	Display_Write_IT(&bars[0], barLength, (0x80 | startingPos));
}
void Display_Clear(void)
{
	Display_Command(DISPLAY_INST_CLEAR_DISPLAY);
}
static void Display_Write_IT(uint8_t* pData, uint8_t numChars, uint8_t command)
{
	if(numChars >= 20)
		numChars = 20;	// Set max length of transmission to 20

	/*
	 * If transmission is already ongoing, add the new data to the stack and
	 * increment the stack pointer.
	 */
	FIFO_buffer[FIFO_head].numChars = numChars;
	FIFO_buffer[FIFO_head].command = command;

	if(numChars != 0)
	{
		DMA2_Stream0->NDTR = numChars;				// Set number of bits to transmit
		DMA2_Stream0->PAR = pData; 					// Set source address
		DMA2_Stream0->M0AR = &(FIFO_buffer[FIFO_head].data[0]);	// Set destination address
		DMA2_Stream0->CR |= DMA_SxCR_EN; 			// Enable the stream
		while((DMA2->LISR & DMA_LISR_TCIF0) == 0){} 	// Wait for DMA transfer to complete
		DMA2->LIFCR |= DMA_LIFCR_CTCIF0;			// Clear flag
		DMA2_Stream0->CR &= ~DMA_SxCR_EN;			// Disable the stream
	}

	if(command == DISPLAY_INST_CLEAR_DISPLAY)
		FIFO_buffer[FIFO_head].delay_us = DISPLAY_CLEAR_DELAY_US;
	else
		FIFO_buffer[FIFO_head].delay_us = DISPLAY_INST_DELAY_US;

	FIFO_head = (FIFO_head + 1) & 0x1F; 		// Increment the head count and restrict to 5 bits

	if(busyFlag)
		return;
	busyFlag = 1;								// Indicate write in progress
	Display_Fast_Write_IT();
}

static void Display_Fast_Write_IT(void)
{
	if(charIndex == 0)
	{
		// If buffer still has elements, transmit the next byte
		if(FIFO_tail != FIFO_head)
		{
			pDataBuf = &FIFO_buffer[FIFO_tail].data[0];
			charIndex = FIFO_buffer[FIFO_tail].numChars;
			GPIOx->ODR &= ~(DISPLAY_DB_Msk 	| 				// Clear output bits
							DISPLAY_RS_PIN 	|
							DISPLAY_RW_PIN	);
			GPIOx->ODR |= 	(FIFO_buffer[FIFO_tail].command << DISPLAY_DB_Pos);	// Set command
			TIM11->ARR =	(uint16_t)(TIMER_FREQ_MHZ * FIFO_buffer[FIFO_tail].delay_us / (TIM11_PRESCALE + 1));	// Set delay time
			Display_Set_E(HIGH);
			TIM11->CR1 |= TIM_CR1_CEN; 				// Start the timer
			FIFO_tail = (FIFO_tail + 1) & 0x1F;		// Increment the tail count and restrict to 4 bits
		}
		else
		{
			busyFlag = 0; 	// Indicate end of transmission
		}
		return;
	}

	GPIOx->ODR &= ~(DISPLAY_DB_Msk 	| 					// Clear output bits
					DISPLAY_RS_PIN 	|
					DISPLAY_RW_PIN	);
	GPIOx->ODR |= 	((*pDataBuf<< DISPLAY_DB_Pos)	|	// Set data bits
					DISPLAY_RS_PIN 	);
	Display_Set_E(HIGH);
	TIM11->CR1 |= TIM_CR1_CEN; 							// Start the timer
	pDataBuf++;											// Increment data pointer
	charIndex--;											// Decrement character counter
}

static void Display_Set_E(uint8_t value)
{
	if(value)
		GPIOx->BSRR = DISPLAY_E_PIN;
	else
		GPIOx->BSRR = (DISPLAY_E_PIN<< GPIO_BSRR_BR0_Pos);
}

void TIM1_TRG_COM_TIM11_IRQHandler(void)
{
	if(TIM11->SR & TIM_SR_CC1IF) 	// If capture event
	{
		TIM11->SR &= ~TIM_SR_CC1IF; // Clear capture flag
		Display_Set_E(LOW);			// Set enable low to latch data
	}
	if(TIM11->SR & TIM_SR_UIF) 		// If overflow event
	{
		TIM11->CR1 &= ~TIM_CR1_CEN; // Stop the timer
		TIM11->EGR |= TIM_EGR_UG;	// Generate update of registers
		TIM11->SR &= ~TIM_SR_UIF; 	// Clear update flag
		Display_Fast_Write_IT();	// Write the next character
	}
}


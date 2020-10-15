/*
 * FLASH.c
 *
 *  Created on: Apr 3, 2018
 *      Author: Ian
 */

#include "FLASH.h"
#define FIFO_BUFFER_LENGTH 64

static uint32_t TIMER_FREQ_MHZ;

typedef struct
{
	uint32_t address;
	uint8_t instruction;
	uint8_t* pData;
} FLASH_Data_TypeDef;

static void FLASH_Write_Enable(void);
static void FLASH_Set_WP_High(void);
static void FLASH_Set_WP_Low(void);
static void FLASH_Set_HOLD_High(void);
static void FLASH_Set_HOLD_Low(void);
static void FLASH_Check_Status(void);
static void FLASH_Page_Write(uint8_t* pData, uint32_t address);
static void FLASH_Page_Read(uint8_t* pData, uint32_t address);
static void FLASH_Erase_Block(uint32_t address);
static void FLASH_Write_Enable(void);
static void FLASH_Next_Instruction(void);

static volatile FLASH_Data_TypeDef FLASH_FIFO_buffer[FIFO_BUFFER_LENGTH];
static volatile uint8_t FLASH_FIFO_tail = 0;
static volatile uint8_t FLASH_FIFO_head = 0;

static volatile uint8_t busyFlag = 0;

static uint8_t txData[4];
static uint8_t rxData[4];

static volatile uint8_t currentInstruction = 0;
void FLASH_Init(uint32_t timerFreqMHz)
{
	TIMER_FREQ_MHZ = timerFreqMHz;

	GPIO_InitTypeDef GPIO_InitStucture;
	GPIO_InitStucture.Pin = 		SPI1_CSn_FLASH_PIN;			// Select chip select pins
	GPIO_InitStucture.Mode = 		GPIO_MODE_OUTPUT_PP;		// Output, push pull mode
	GPIO_InitStucture.Speed =		GPIO_SPEED_FREQ_VERY_HIGH ;	// Set speed for 50Mhz-200MHz
	GPIO_InitStucture.Pull = 		GPIO_NOPULL;				// No pullups or pulldowns
	FLASH_Set_CS_High();										// Set CS pin to high by default
	HAL_GPIO_Init(SPI1_CSn_FLASH_PORT, &GPIO_InitStucture);		// Initialize

	GPIO_InitStucture.Pin = 		FLASH_WPn_PIN;				// Select chip select pins
	GPIO_InitStucture.Mode = 		GPIO_MODE_OUTPUT_PP;		// Output, push pull mode
	GPIO_InitStucture.Speed =		GPIO_SPEED_FREQ_VERY_HIGH ;	// Set speed for 50Mhz-200MHz
	GPIO_InitStucture.Pull = 		GPIO_NOPULL;				// No pullups or pulldowns
	FLASH_Set_WP_High();										// Set WP pin to high by default
	HAL_GPIO_Init(FLASH_WPn_PORT, &GPIO_InitStucture);			// Initialize

	GPIO_InitStucture.Pin = 		FLASH_HOLDn_PIN;			// Select chip select pins
	GPIO_InitStucture.Mode = 		GPIO_MODE_OUTPUT_PP;		// Output, push pull mode
	GPIO_InitStucture.Speed =		GPIO_SPEED_FREQ_VERY_HIGH ;	// Set speed for 50Mhz-200MHz
	GPIO_InitStucture.Pull = 		GPIO_NOPULL;				// No pullups or pulldowns
	FLASH_Set_HOLD_High();										// Set HOLD pin to high by default
	HAL_GPIO_Init(FLASH_HOLDn_PORT, &GPIO_InitStucture);		// Initialize

	// Set up TIM14
	TIM14->CR1 |= (	TIM_CR1_ARPE 	|			// Enable auto-preload
					TIM_CR1_URS);				// Only overflow generates interrupt
	TIM14->DIER |= 	TIM_DIER_UIE;				// Generate interrupt on overflow
	TIM14->PSC = 	TIMER_FREQ_MHZ;				// Prescale the timer to 1 us per tick
	TIM14->ARR = FLASH_STATUS_CHECK_TIME_US;	// Set reload value
	TIM14->EGR |= TIM_EGR_UG;					// Generate update of registers
	TIM14->SR &= ~TIM_SR_UIF; 					// Clear update flag

	NVIC_SetPriority(TIM8_TRG_COM_TIM14_IRQn,6);// Set interrupt priority
	NVIC_EnableIRQ(TIM8_TRG_COM_TIM14_IRQn);	// Enable overflow interrupt

	EXTI->IMR |= EXTI_IMR_MR3;				// Enable interrupt generated on EXTI3
	NVIC_SetPriority(EXTI3_IRQn,4);			// Set interrupt priority
	NVIC_EnableIRQ(EXTI3_IRQn);				// Enable EXTI2 interrupt
}

void FLASH_Test(void)
{
//	TIM10->ARR = 0xFFFF;
//	TIM10->PSC = 800;
//	TIM10->EGR |= TIM_EGR_UG;				// Generate update of registers
//	TIM10->SR &= ~TIM_SR_UIF; 				// Clear update flag
//	TIM10->CR1 |= TIM_CR1_CEN;



	//uint16_t time = TIM10->CNT;
	uint32_t i;
	uint8_t data[260] = {0x02, 0x00, 0x00, 0x00};
	for(i = 4; i < 260; i++)
	{
		data[i] = i-4;
	}

	FLASH_Erase_Blocks_IT(0, 256);


	FLASH_Page_Write_IT(&data[0], 0);

	uint8_t RxData[260] = {0, 0, 0, 0, 0};
	FLASH_Page_Read_IT(&RxData[0], 0);

	while(busyFlag){}
	i = 1;
}

void FLASH_Page_Write_IT(uint8_t* pData, uint32_t address)
{

	FLASH_FIFO_buffer[FLASH_FIFO_head].address = address;
	FLASH_FIFO_buffer[FLASH_FIFO_head].pData = pData;
	FLASH_FIFO_buffer[FLASH_FIFO_head].instruction = FLASH_INST_PAGE_PROGRAM;
	FLASH_FIFO_head = (FLASH_FIFO_head + 1) & (FIFO_BUFFER_LENGTH - 1); 			// Increment head count and wrap around if overflow

	if(busyFlag == 0)
	{
		busyFlag = 1;
		FLASH_Next_Instruction();
	}

}

void FLASH_Page_Read_IT(uint8_t* pData, uint32_t address)
{
	FLASH_FIFO_buffer[FLASH_FIFO_head].address = address;
	FLASH_FIFO_buffer[FLASH_FIFO_head].pData = pData;
	FLASH_FIFO_buffer[FLASH_FIFO_head].instruction = FLASH_INST_READ_DATA;
	FLASH_FIFO_head = (FLASH_FIFO_head + 1) & (FIFO_BUFFER_LENGTH - 1); 			// Increment head count and wrap around if overflow

	if(busyFlag == 0)
	{
		busyFlag = 1;
		FLASH_Next_Instruction();
	}

}

void FLASH_Erase_Blocks_IT(uint32_t startAddress, uint32_t endAddress)
{
	while(startAddress <= endAddress)
	{
		FLASH_FIFO_buffer[FLASH_FIFO_head].instruction = FLASH_INST_BLOCK_ERASE_64;
		FLASH_FIFO_buffer[FLASH_FIFO_head].address = startAddress;

		startAddress += FLASH_BLOCK_64_SIZE;								// Increment start address to next block
		FLASH_FIFO_head = (FLASH_FIFO_head + 1) & (FIFO_BUFFER_LENGTH - 1); 			// Increment head count and wrap around if overflow
	}

	if(busyFlag == 0)
	{
		busyFlag = 1;
		FLASH_Next_Instruction();
	}
}

static void FLASH_Next_Instruction(void)
{
	if(FLASH_FIFO_head == FLASH_FIFO_tail)			// If there is no instruction pending
	{
		busyFlag = 0;					// Reset busy flag
		return;							// Exit
	}

	switch(FLASH_FIFO_buffer[FLASH_FIFO_tail].instruction)
	{
	case FLASH_INST_PAGE_PROGRAM:
		FLASH_Write_Enable();
		break;
	case FLASH_INST_READ_DATA:
		FLASH_Page_Read(FLASH_FIFO_buffer[FLASH_FIFO_tail].pData, FLASH_FIFO_buffer[FLASH_FIFO_tail].address);
		break;
	case FLASH_INST_BLOCK_ERASE_64:
		FLASH_Write_Enable();
		break;
	}
}

static void FLASH_Write_Enable(void)
{
	currentInstruction = FLASH_INST_WRITE_ENABLE;
	txData[0] = FLASH_INST_WRITE_ENABLE;				// Send write enable instruction
	FLASH_Set_CS_Low();									// Set CS low
	SPI1_Read_Write(&rxData[0],&txData[0], 1);			// Send the data
}

static void FLASH_Page_Write(uint8_t* pData, uint32_t address)
{
	currentInstruction = FLASH_INST_PAGE_PROGRAM;
	pData[3] = (uint8_t)(address);
	pData[2] = (uint8_t)(address >> 8);
	pData[1] = (uint8_t)(address >> 16);
	pData[0] = FLASH_INST_PAGE_PROGRAM;
	FLASH_Set_CS_Low();
	SPI1_Read_Write(pData, pData, FLASH_PAGE_SIZE + 4);
}

static void FLASH_Page_Read(uint8_t* pData, uint32_t address)
{
	currentInstruction = FLASH_INST_READ_DATA;
	pData[3] = (uint8_t)(address);
	pData[2] = (uint8_t)(address >> 8);
	pData[1] = (uint8_t)(address >> 16);
	pData[0] = FLASH_INST_READ_DATA;
	FLASH_Set_CS_Low();
	SPI1_Read_Write(pData, pData, FLASH_PAGE_SIZE + 4);
}

static void FLASH_Erase_Block(uint32_t address)
{
	currentInstruction = FLASH_INST_BLOCK_ERASE_64;
	txData[3] = 0;
	txData[2] = 0;
	txData[1] = (uint8_t)(address >> 16);
	txData[0] = FLASH_INST_BLOCK_ERASE_64;
	FLASH_Set_CS_Low();
	SPI1_Read_Write(&rxData[0], &txData[0], 4);
}

static void FLASH_Check_Status(void)
{
	currentInstruction = FLASH_INST_READ_STATUS_1;
	txData[0] = FLASH_INST_READ_STATUS_1;
	FLASH_Set_CS_Low();
	SPI1_Read_Write(&rxData[0], &txData[0], 2);
}

void FLASH_End_Transmission(void)
{
	switch(currentInstruction)
	{
	case FLASH_INST_WRITE_ENABLE:
		switch(FLASH_FIFO_buffer[FLASH_FIFO_tail].instruction)
		{
		case FLASH_INST_PAGE_PROGRAM:
			FLASH_Page_Write(FLASH_FIFO_buffer[FLASH_FIFO_tail].pData, FLASH_FIFO_buffer[FLASH_FIFO_tail].address);
			break;
		case FLASH_INST_BLOCK_ERASE_64:
			FLASH_Erase_Block(FLASH_FIFO_buffer[FLASH_FIFO_tail].address);
			break;
		}
		break;
	case FLASH_INST_READ_STATUS_1:
		if((rxData[1] & 0x01) == 0)				// Check if chip is busy
		{
			FLASH_FIFO_tail = (FLASH_FIFO_tail + 1) & (FIFO_BUFFER_LENGTH - 1); 		// Increment tail count when finished with last instruction
			FLASH_Next_Instruction();				// Next instruction if not busy
		}
		else
		{
			TIM14->EGR |= TIM_EGR_UG;				// Generate update of registers
			TIM14->SR &= ~TIM_SR_UIF; 				// Clear update flag
			TIM14->CR1 |= TIM_CR1_CEN; 				// Start the timer to check status again later
		}
		break;
	case FLASH_INST_BLOCK_ERASE_64:
		FLASH_Check_Status();														// Check the chip status after an erase
		break;
	default:
		FLASH_FIFO_tail = (FLASH_FIFO_tail + 1) & (FIFO_BUFFER_LENGTH - 1); 		// Increment tail count when finished with last instruction
		FLASH_Next_Instruction();
		break;
	}
}

uint8_t FLASH_Get_Busy_Flag(void)
{
	return busyFlag;
}
void FLASH_Set_CS_High(void)
{
	SPI1_CSn_FLASH_PORT->BSRR =  	SPI1_CSn_FLASH_PIN;
}
void FLASH_Set_CS_Low(void)
{
	SPI1_CSn_FLASH_PORT->BSRR =  	(SPI1_CSn_FLASH_PIN << GPIO_BSRR_BR0_Pos);
}
static void FLASH_Set_WP_High(void)
{
	FLASH_WPn_PORT->BSRR =  	FLASH_WPn_PIN;
}
static void FLASH_Set_WP_Low(void)
{
	FLASH_WPn_PORT->BSRR =  	(FLASH_WPn_PIN << GPIO_BSRR_BR0_Pos);
}
static void FLASH_Set_HOLD_High(void)
{
	FLASH_HOLDn_PORT->BSRR =  	FLASH_HOLDn_PIN;
}
static void FLASH_Set_HOLD_Low(void)
{
	FLASH_HOLDn_PORT->BSRR =  	(FLASH_HOLDn_PIN << GPIO_BSRR_BR0_Pos);
}

void TIM8_TRG_COM_TIM14_IRQHandler(void)
{
	if(TIM14->SR & TIM_SR_UIF) 		// If overflow event
	{
		TIM14->CR1 &= ~TIM_CR1_CEN; // Stop the timer
		TIM14->SR &= ~TIM_SR_UIF; 	// Clear update flag
		FLASH_Check_Status();
	}
}
void EXTI3_IRQHandler(void)
{
	if(EXTI->PR & EXTI_PR_PR3)
	{
		EXTI->PR |= EXTI_PR_PR3; // Reset flag
		FLASH_End_Transmission();
	}
}

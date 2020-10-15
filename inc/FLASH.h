/*
 * FLASH.h
 *
 *  Created on: Apr 3, 2018
 *      Author: Ian
 */

#ifndef FLASH_H_
#define FLASH_H_

#include "stm32F4xx.h"
#include "stm32f4xx_hal.h"
#include "SPI1.h"

#define SPI1_CSn_FLASH_PORT	GPIOA
#define SPI1_CSn_FLASH_PIN 	GPIO_PIN_4
#define FLASH_WPn_PORT		GPIOC
#define FLASH_WPn_PIN 		GPIO_PIN_11
#define FLASH_HOLDn_PORT	GPIOC
#define FLASH_HOLDn_PIN 	GPIO_PIN_12

#define FLASH_INST_WRITE_ENABLE		0x06
#define FLASH_INST_READ_DATA		0x03
#define FLASH_INST_PAGE_PROGRAM		0x02
#define FLASH_INST_SECTOR_ERASE_4	0x20
#define FLASH_INST_BLOCK_ERASE_32	0x52
#define FLASH_INST_BLOCK_ERASE_64	0xD8
#define FLASH_INST_CHIP_ERASE		0xC7
#define FLASH_INST_READ_STATUS_1 	0x05

#define FLASH_PAGE_SIZE				256
#define FLASH_BLOCK_64_SIZE			0x10000

#define FLASH_STATUS_CHECK_TIME_US	1000

void FLASH_Init(uint32_t timerFreqMHz);
void FLASH_Test(void);
void FLASH_End_Transmission(void);
void FLASH_Page_Write_IT(uint8_t* pData, uint32_t address);
void FLASH_Page_Read_IT(uint8_t* pData, uint32_t address);
void FLASH_Erase_Blocks_IT(uint32_t startAddress, uint32_t endAddress);
uint8_t FLASH_Get_Busy_Flag(void);

void FLASH_Set_CS_High(void);
void FLASH_Set_CS_Low(void);


#endif /* FLASH_H_ */

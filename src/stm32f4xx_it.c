/**
  ******************************************************************************
  * @file    stm32f4xx_it.c
  * @author  Ac6
  * @version V1.0
  * @date    02-Feb-2015
  * @brief   Default Interrupt Service Routines.
  ******************************************************************************
*/

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"
#include "stm32f4xx.h"
#ifdef USE_RTOS_SYSTICK
#include <cmsis_os.h>
#endif
#include "stm32f4xx_it.h"

#include "ADC.h"
#include "DAC.h"
#include "FLASH.h"
#include "SPI2.h"
#include "SPI1.h"
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/******************************************************************************/
/*            	  	    Processor Exceptions Handlers                         */
/******************************************************************************/

/**
  * @brief  This function handles SysTick Handler, but only if no RTOS defines it.
  * @param  None
  * @retval None
  */
void SysTick_Handler(void)
{
	HAL_IncTick();
	HAL_SYSTICK_IRQHandler();
#ifdef USE_RTOS_SYSTICK
	osSystickHandler();
#endif
}


void DMA1_Stream3_IRQHandler(void)
{
	if(DMA1->LISR & DMA_LISR_TCIF3)
	{
		DMA1->LIFCR |= DMA_LIFCR_CTCIF3; // Clear DMA stream 3 interrupt
		ADC_Set_Complete_Flag();
		DAC_Set_Complete_Flag();
		SPI2_Disable();
	}
}


// DMA2 RX Complete
void DMA2_Stream2_IRQHandler(void)
{
	if(DMA2->LISR & DMA_LISR_TCIF2)
	{
		FLASH_Set_CS_High();
		SPI1_Disable();
		SPI1_Set_Complete_Flag();
		EXTI->SWIER |= EXTI_SWIER_SWIER3; 	// Generate EXTI3 interrupt
		DMA2->LIFCR |= DMA_LIFCR_CTCIF2; 	// Clear DMA stream 2 interrupt
	}
}

void DMA2_Stream3_IRQHandler(void)
{
	if(DMA2->LISR & DMA_LISR_TCIF3)
	{
		DMA2->LIFCR |= DMA_LIFCR_CTCIF3; // Clear DMA stream 3 interrupt
	}
}

/* The prototype shows it is a naked function - in effect this is just an
assembly function. */
void HardFault_Handler( void ) __attribute__( ( naked ) );

/* The fault handler implementation calls a function called
prvGetRegistersFromStack(). */
void HardFault_Handler(void)
{
    __asm volatile
    (
    	" mov r4, #0xE000E000										\n"
    	" add r4, #0xD28											\n"
    	" ldr r3, [r4, #0]											\n"
        " tst lr, #4                                                \n"
        " ite eq                                                    \n"
        " mrseq r0, msp                                             \n"
        " mrsne r0, psp                                             \n"
        " ldr r1, [r0, #24]                                         \n"
        " ldr r2, handler2_address_const                            \n"
        " bx r2                                                     \n"
        " handler2_address_const: .word prvGetRegistersFromStack    \n"
    );
}

void prvGetRegistersFromStack( uint32_t *pulFaultStackAddress )
{
/* These are volatile to try and prevent the compiler/linker optimising them
away as the variables never actually get used.  If the debugger won't show the
values of the variables, make them global my moving their declaration outside
of this function. */
volatile uint32_t r0;
volatile uint32_t r1;
volatile uint32_t r2;
volatile uint32_t r3;
volatile uint32_t r12;
volatile uint32_t lr; /* Link register. */
volatile uint32_t pc; /* Program counter. */
volatile uint32_t psr;/* Program status register. */

    r0 = pulFaultStackAddress[ 0 ];
    r1 = pulFaultStackAddress[ 1 ];
    r2 = pulFaultStackAddress[ 2 ];
    r3 = pulFaultStackAddress[ 3 ];

    r12 = pulFaultStackAddress[ 4 ];
    lr = pulFaultStackAddress[ 5 ];
    pc = pulFaultStackAddress[ 6 ];
    psr = pulFaultStackAddress[ 7 ];

    /* When the following line is hit, the variables contain the register values. */
    for( ;; );
}

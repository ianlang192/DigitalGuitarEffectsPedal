/*
 * clk_init.c
 *
 *  Created on: Apr 5, 2018
 *      Author: Ian
 */

#include "clk_init.h"

/*
 * System Clock Configuration
*/
void SystemClock_Config(void)
{

	RCC_OscInitTypeDef RCC_OscInitStruct;
	RCC_ClkInitTypeDef RCC_ClkInitStruct;

	/**Configure the main internal regulator output voltage
	 */
	__HAL_RCC_PWR_CLK_ENABLE();

	__HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

	/**Initializes the CPU, AHB and APB busses clocks
	 */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
	RCC_OscInitStruct.HSEState = RCC_HSE_ON;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
	RCC_OscInitStruct.PLL.PLLM = 15;
	RCC_OscInitStruct.PLL.PLLN = 192;
	RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
	RCC_OscInitStruct.PLL.PLLQ = 2;
	RCC_OscInitStruct.PLL.PLLR = 2;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
	{
		//_Error_Handler(__FILE__, __LINE__);
	}

	/**Initializes the CPU, AHB and APB busses clocks
	 */
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
			|RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV4;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
	{
		//_Error_Handler(__FILE__, __LINE__);
	}

	/**Configure the Systick interrupt time
	 */
	HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/1000);

	/**Configure the Systick
	 */
	HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);

	/* SysTick_IRQn interrupt configuration */
	HAL_NVIC_SetPriority(SysTick_IRQn, 0, 0);

	// Enable peripheral clocks
	RCC->AHB1ENR |= (	RCC_AHB1ENR_GPIOAEN |	// GPIOA
						RCC_AHB1ENR_GPIOBEN |	// GPIOB
						RCC_AHB1ENR_GPIOCEN |	// GPIOC
						RCC_AHB1ENR_GPIODEN |	// GPIOD
						RCC_AHB1ENR_DMA1EN	|	// DMA1
						RCC_AHB1ENR_DMA2EN	);	// DMA2
	RCC->APB1ENR |= (	RCC_APB1ENR_SPI2EN	|	// SPI2
						RCC_APB1ENR_TIM6EN	|	// TIM6
						RCC_APB1ENR_TIM7EN	|	// TIM7
						RCC_APB1ENR_TIM2EN	|	// TIM2
						RCC_APB1ENR_TIM3EN 	|	// TIM3
						RCC_APB1ENR_TIM12EN	|	// TIM12
						RCC_APB1ENR_TIM13EN	|	// TIM13
						RCC_APB1ENR_TIM14EN	);	// TIM14
	RCC->APB2ENR |= (	RCC_APB2ENR_TIM1EN	|	// TIM1
						RCC_APB2ENR_SYSCFGEN|	// SYSCONFIG
						RCC_APB2ENR_TIM9EN	|	// TIM9
						RCC_APB2ENR_TIM10EN	|	// TIM10
						RCC_APB2ENR_TIM11EN	|	// TIM11
						RCC_APB2ENR_SPI1EN);	// SPI1
}

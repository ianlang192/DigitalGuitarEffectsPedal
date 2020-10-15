/*
 * front_panel.c
 *
 *  Created on: Apr 28, 2018
 *      Author: Ian
 */

#include "front_panel.h"

#define TIM12_PRESCALE	40000
#define TIM13_PRESCALE	40000

static uint32_t TIMER_FREQ_MHZ;
static volatile uint32_t rotation = 0; // 0 if no rotation, 1 if left, 2 if right

static volatile uint8_t event = 0;
static volatile uint8_t buttonStatus = 0;

static uint8_t Process_Button_Press(uint8_t buttonNum);

void Buttons_Init(uint32_t timerFreqMHz)
{
	TIMER_FREQ_MHZ = timerFreqMHz;

	GPIO_InitTypeDef GPIO_InitStucture;

	GPIO_InitStucture.Mode = 		GPIO_MODE_INPUT;				// Input mode
	GPIO_InitStucture.Speed =		GPIO_SPEED_FREQ_MEDIUM;			// Set speed to medium
	GPIO_InitStucture.Pull = 		GPIO_PULLUP;					// Enable pullups

	GPIO_InitStucture.Pin = BUTTON_MENU_UP_PIN;						// Select button pin
	HAL_GPIO_Init(BUTTON_MENU_UP_PORT, &GPIO_InitStucture);			// Initialize
	GPIO_InitStucture.Pin = BUTTON_MENU_OK_PIN;						// Select button pin
	HAL_GPIO_Init(BUTTON_MENU_OK_PORT, &GPIO_InitStucture);			// Initialize
	GPIO_InitStucture.Pin = BUTTON_MENU_DOWN_PIN;					// Select button pin
	HAL_GPIO_Init(BUTTON_MENU_DOWN_PORT, &GPIO_InitStucture);		// Initialize
	GPIO_InitStucture.Pin = BUTTON_STOMP_UP_PIN;					// Select button pin
	HAL_GPIO_Init(BUTTON_STOMP_UP_PORT, &GPIO_InitStucture);		// Initialize
	GPIO_InitStucture.Pin = BUTTON_STOMP_DOWN_PIN;					// Select button pin
	HAL_GPIO_Init(BUTTON_STOMP_DOWN_PORT, &GPIO_InitStucture);		// Initialize
	GPIO_InitStucture.Pin = BUTTON_STOMP_EFFECT_PIN;				// Select button pin
	HAL_GPIO_Init(BUTTON_STOMP_EFFECT_PORT, &GPIO_InitStucture);	// Initialize

	// Map external interrupt vectors
	SYSCFG->EXTICR[0] |= (	SYSCFG_EXTICR1_EXTI0_PB	|
							SYSCFG_EXTICR1_EXTI1_PB	);
	SYSCFG->EXTICR[1] |= (	SYSCFG_EXTICR2_EXTI5_PB	|
							SYSCFG_EXTICR2_EXTI6_PB	);
	SYSCFG->EXTICR[2] |= (	SYSCFG_EXTICR3_EXTI8_PA	|
							SYSCFG_EXTICR3_EXTI10_PA);
	// Enable falling edge trigger
	EXTI->FTSR |= (			EXTI_FTSR_TR0	|
							EXTI_FTSR_TR1	|
							EXTI_FTSR_TR5	|
							EXTI_FTSR_TR6	|
							EXTI_FTSR_TR8	|
							EXTI_FTSR_TR10);

	// Unmask external interrupt lines
	EXTI->IMR |= (			EXTI_IMR_IM0	|
							EXTI_IMR_IM1	|
							EXTI_IMR_IM5	|
							EXTI_IMR_IM6	|
							EXTI_IMR_IM8	|
							EXTI_IMR_IM10);

	// Enable interrupts
	NVIC_SetPriority(EXTI0_IRQn, 6);
	NVIC_SetPriority(EXTI1_IRQn, 6);
	NVIC_SetPriority(EXTI9_5_IRQn, 6);
	NVIC_SetPriority(EXTI15_10_IRQn, 6);

	NVIC_EnableIRQ(EXTI0_IRQn);
	NVIC_EnableIRQ(EXTI1_IRQn);
	NVIC_EnableIRQ(EXTI9_5_IRQn);
	NVIC_EnableIRQ(EXTI15_10_IRQn);

	TIM12->CR1 |= (	TIM_CR1_ARPE 	|		// Enable auto-preload
					TIM_CR1_OPM 	|		// Set one pulse mode
					TIM_CR1_URS);			// Only overflow generates interrupt
	TIM12->DIER |= 	TIM_DIER_UIE;			// Generate interrupt on overflow
	TIM12->PSC = 	TIM12_PRESCALE;			// Prescale the timer
	TIM12->ARR = (TIMER_FREQ_MHZ * BUTTON_LONG_PRESS_MS * 1000) / (TIM12_PRESCALE);	// Calculate required timer value
	TIM12->EGR |= TIM_EGR_UG;				// Generate update of registers
	TIM12->SR &= ~TIM_SR_UIF; 				// Clear update flag

	TIM13->CR1 |= (	TIM_CR1_ARPE 	|		// Enable auto-preload
					TIM_CR1_OPM 	|		// Set one pulse mode
					TIM_CR1_URS);			// Only overflow generates interrupt
	TIM13->DIER |= 	TIM_DIER_UIE;			// Generate interrupt on overflow
	TIM13->PSC = 	TIM13_PRESCALE;			// Prescale the timer
	TIM13->ARR = (TIMER_FREQ_MHZ * BUTTON_LOCKOUT_MS * 1000) / (TIM13_PRESCALE);	// Calculate required timer value
	TIM13->CR1 |= 	TIM_CR1_CEN;			// Enable timer
}

void Encoder_Init(uint32_t timerFreqMHz)
{
	TIMER_FREQ_MHZ = timerFreqMHz;

	// Initialize GPIO pins
	GPIO_InitTypeDef GPIO_InitStucture;
	GPIO_InitStucture.Pin = (		ENCODER_A_PIN |			// Select encoder pins
									ENCODER_B_PIN);
	GPIO_InitStucture.Mode = 		GPIO_MODE_AF_PP;		// Alternate function, push pull mode
	GPIO_InitStucture.Speed =		GPIO_SPEED_FREQ_MEDIUM; // Set speed to medium
	GPIO_InitStucture.Pull = 		GPIO_PULLUP;			// Enable pullup resistors
	GPIO_InitStucture.Alternate = 	GPIO_AF1_TIM2;			// Set alternate function to AF1
	HAL_GPIO_Init(ENCODER_PORT, &GPIO_InitStucture);		// Initialize

	TIM2->CR1 |= 		TIM_CR1_CKD_1;			// f_DTS = f_CK_INT / 4
	TIM2->CCMR1 |= (	TIM_CCMR1_CC1S_0	|	// TI1FP1 mapped to TI1
						TIM_CCMR1_IC1F		|	// Set digital filter, fsample = f_DTS/32, N=8
						TIM_CCMR1_CC2S_0	|	// TI2FP2 mapped to TI2
						TIM_CCMR1_IC2F);		// Set digital filter, fsample = f_DTS/32, N=8
	TIM2->SMCR |= (		TIM_SMCR_SMS_0		|	// Select encoder mode to count on both TI1 and TI2
						TIM_SMCR_SMS_1);
	TIM2->DIER |= (		TIM_DIER_CC3IE		|	// Enable interrupt on CC3
						TIM_DIER_CC4IE);		// Enable interrupt on CC4
	TIM2->ARR =			0xFFFF;					// Set auto reload value to max
	TIM2->CCR3 =		TIM2->CNT - ENCODER_TICK_CNT;
	TIM2->CCR4 =		TIM2->CNT + ENCODER_TICK_CNT;
	TIM2->EGR |=		TIM_EGR_UG;				// Generate update of registers

	NVIC_SetPriority(TIM2_IRQn,6); 				// Set interrupt priority
	NVIC_EnableIRQ(TIM2_IRQn);					// Enable interrupts

	TIM2->CR1 |= 		TIM_CR1_CEN;			// Enable the timer
}

uint8_t Front_Panel_Encoder_Rotation(void)
{
	return rotation;
}

void Front_Panel_Reset_Encoder_Rotation(void)
{
	rotation = 0;
}
uint8_t Front_Panel_Button_Status(void)
{
	return buttonStatus;
}

void Reset_Front_Panel_Button_Status()
{
	buttonStatus = 0;
}

uint8_t Front_Panel_Event(void)
{
	return event;
}
void Clear_Front_Panel_Event(void)
{
	event = 0;
}
void TIM2_IRQHandler(void)
{
	if(TIM2->SR & TIM_SR_CC3IF)
	{
		TIM2->SR &= ~TIM_SR_CC3IF; // Reset flag
		TIM2->CCR3 = TIM2->CNT - ENCODER_TICK_CNT;
		TIM2->CCR4 = TIM2->CNT + ENCODER_TICK_CNT;
		rotation = 1;
		event = 1;
	}
	if(TIM2->SR & TIM_SR_CC4IF)
	{
		TIM2->SR &= ~TIM_SR_CC4IF; // Reset flag
		TIM2->CCR3 = TIM2->CNT - ENCODER_TICK_CNT;
		TIM2->CCR4 = TIM2->CNT + ENCODER_TICK_CNT;
		rotation = 2;
		event = 1;
	}
}

/*
 * Returns 0 if button was pressed
 * Returns 1 if button was released after short press
 * Returns 2 if button was released after long press
 */
static uint8_t Process_Button_Press(uint8_t buttonNum)
{
	if(EXTI->FTSR & (1 << buttonNum))		// If the falling edge trigger is set, then the button is not held down
	{
		EXTI->RTSR |= (1 << buttonNum);		// Enable rising edge trigger
		EXTI->FTSR &= ~(1 << buttonNum);	// Disable falling edge trigger
		TIM12->EGR |= TIM_EGR_UG;			// Reset timer
		TIM12->SR &= ~TIM_SR_UIF;			// Reset flag
		TIM12->CR1 |= TIM_CR1_CEN;			// Enable timer
		return 0;
	}
	else
	{
		EXTI->FTSR |= (1 << buttonNum);		// Enable falling edge trigger
		EXTI->RTSR &= ~(1 << buttonNum);	// Disable rising edge trigger
		if(TIM12->SR & TIM_SR_UIF)			// If button has been held for long enough
		{
			TIM12->SR &= ~TIM_SR_UIF;		// Reset flag
			return 2;
		}
		else
		{
			return 1;
		}
	}
}

/*
 * Menu down button (EXTI0)
 */
void EXTI0_IRQHandler(void)
{
	if(EXTI->PR & EXTI_PR_PR0)
	{
		EXTI->PR |= EXTI_PR_PR0; // Reset flag
		if(TIM13->SR & TIM_SR_UIF)	// Ignore button presses within set time
		{
			TIM13->EGR |= TIM_EGR_UG;			// Reset timer
			TIM13->SR &= ~TIM_SR_UIF;			// Reset flag
			TIM13->CR1 |= TIM_CR1_CEN;			// Enable timer
		}
		else
		{
			return;
		}
		uint8_t buttonResult = Process_Button_Press(BUTTON_MENU_DOWN_Pos);
		if(buttonResult == 1)
		{
			buttonStatus = BUTTON_STATUS_MENU_DOWN_PRESS;
			event = 1;
		}
		else if(buttonResult == 2)
		{
			buttonStatus = BUTTON_STATUS_MENU_DOWN_HOLD;
			event = 1;
		}
	}
}

/*
 * Stomp up button (EXTI1)
 */
void EXTI1_IRQHandler(void)
{
	if(EXTI->PR & EXTI_PR_PR1)
	{
		EXTI->PR |= EXTI_PR_PR1; // Reset flag
		if(TIM13->SR & TIM_SR_UIF)	// Ignore button presses within set time
		{
			TIM13->EGR |= TIM_EGR_UG;			// Reset timer
			TIM13->SR &= ~TIM_SR_UIF;			// Reset flag
			TIM13->CR1 |= TIM_CR1_CEN;			// Enable timer
		}
		else
			return;
		uint8_t buttonResult = Process_Button_Press(BUTTON_STOMP_UP_Pos);
		if(buttonResult == 1)
		{
			buttonStatus = BUTTON_STATUS_STOMP_UP_PRESS;
			event = 1;
		}
		else if(buttonResult == 2)
		{
			buttonStatus = BUTTON_STATUS_STOMP_UP_HOLD;
			event = 1;
		}
	}
}

/*
 * Stomp down button (EXTI5)
 * Stomp effect button (EXTI6)
 * Menu up button (EXTI8)
 */
void EXTI9_5_IRQHandler(void)
{
	if(EXTI->PR & EXTI_PR_PR5)
	{
		EXTI->PR |= EXTI_PR_PR5; // Reset flag
		if(TIM13->SR & TIM_SR_UIF)	// Ignore button presses within set time
		{
			TIM13->EGR |= TIM_EGR_UG;			// Reset timer
			TIM13->SR &= ~TIM_SR_UIF;			// Reset flag
			TIM13->CR1 |= TIM_CR1_CEN;			// Enable timer
		}
		else
			return;
		uint8_t buttonResult = Process_Button_Press(BUTTON_STOMP_DOWN_Pos);
		if(buttonResult == 1)
		{
			buttonStatus = BUTTON_STATUS_STOMP_DOWN_PRESS;
			event = 1;
		}
		else if(buttonResult == 2)
		{
			buttonStatus = BUTTON_STATUS_STOMP_DOWN_HOLD;
			event = 1;
		}
	}
	if(EXTI->PR & EXTI_PR_PR6)
	{
		EXTI->PR |= EXTI_PR_PR6; // Reset flag
		if(TIM13->SR & TIM_SR_UIF)	// Ignore button presses within set time
		{
			TIM13->EGR |= TIM_EGR_UG;			// Reset timer
			TIM13->SR &= ~TIM_SR_UIF;			// Reset flag
			TIM13->CR1 |= TIM_CR1_CEN;			// Enable timer
		}
		else
			return;
		uint8_t buttonResult = Process_Button_Press(BUTTON_STOMP_EFFECT_Pos);
		if(buttonResult == 1)
		{
			buttonStatus = BUTTON_STATUS_STOMP_EFFECT_PRESS;
			event = 1;
		}
		else if(buttonResult == 2)
		{
			buttonStatus = BUTTON_STATUS_STOMP_EFFECT_HOLD;
			event = 1;
		}
	}
	if(EXTI->PR & EXTI_PR_PR8)
	{
		EXTI->PR |= EXTI_PR_PR8; // Reset flag
		if(TIM13->SR & TIM_SR_UIF)	// Ignore button presses within set time
		{
			TIM13->EGR |= TIM_EGR_UG;			// Reset timer
			TIM13->SR &= ~TIM_SR_UIF;			// Reset flag
			TIM13->CR1 |= TIM_CR1_CEN;			// Enable timer
		}
		else
			return;
		uint8_t buttonResult = Process_Button_Press(BUTTON_MENU_UP_Pos);
		if(buttonResult == 1)
		{
			buttonStatus = BUTTON_STATUS_MENU_UP_PRESS;
			event = 1;
		}
		else if(buttonResult == 2)
		{
			buttonStatus = BUTTON_STATUS_MENU_UP_HOLD;
			event = 1;
		}
	}
}

/*
 * Menu ok button (EXTI10)
 */
void EXTI15_10_IRQHandler(void)
{
	if(EXTI->PR & EXTI_PR_PR10)
	{
		EXTI->PR |= EXTI_PR_PR10; // Reset flag
		if(TIM13->SR & TIM_SR_UIF)	// Ignore button presses within set time
		{
			TIM13->EGR |= TIM_EGR_UG;			// Reset timer
			TIM13->SR &= ~TIM_SR_UIF;			// Reset flag
			TIM13->CR1 |= TIM_CR1_CEN;			// Enable timer
		}
		else
			return;
		uint8_t buttonResult = Process_Button_Press(BUTTON_MENU_OK_Pos);
		if(buttonResult == 1)
		{
			buttonStatus = BUTTON_STATUS_MENU_OK_PRESS;
			event = 1;
		}
		else if(buttonResult == 2)
		{
			buttonStatus = BUTTON_STATUS_MENU_OK_HOLD;
			event = 1;
		}
	}
}

/*
 * audio.c
 *
 *  Created on: Apr 5, 2018
 *      Author: Ian
 */

#include "audio.h"


#define NOTE_CALCULATION_DENOMINATOR 0.0251

#define TUNER_REFERENCE_FREQ_DEFAULT 	440
#define TUNER_REFERENCE_FREQ_MIN 		400
#define TUNER_REFERENCE_FREQ_MAX 		480



static char* NOTE_NAMES[] = {"A", "A#", "B", "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#"};

static uint32_t TIMER_FREQ_MHZ;


volatile static uint16_t* pInputBuffer;
static uint16_t* pInputBufferStart;
static uint16_t* pInputBufferEnd;

static uint16_t* pOutputBuffer;

static float32_t* pTunerInputBuffer;
static float32_t* pTunerResultBuffer;

volatile static uint8_t tunerReady = 0;
volatile static uint8_t tunerOutput = 1;

static float32_t* pTunerResultPeriod;
static float32_t* pTunerResultPeriodStart;
static float32_t* pTunerResultPeriodEnd;


volatile static float32_t detectedNote = 0;
static uint16_t tunerReferenceFreq = TUNER_REFERENCE_FREQ_DEFAULT;

static uint16_t* pLooperBuffer;
static uint16_t* pLooperBufferStart;
static uint16_t* pLooperBufferEnd;
static uint16_t* pLooperBufferInstructionStart;

static uint32_t looperCurrentFlashAddress = LOOPER_STARTING_FLASH_ADDRESS;
static uint32_t looperEndFlashAddress = LOOPER_STARTING_FLASH_ADDRESS;

static volatile uint8_t audioState = 0;

void Audio_Init(uint32_t timerFreqMHz)
{
	TIMER_FREQ_MHZ = timerFreqMHz;
	pInputBufferStart = malloc(AUDIO_BUFFER_SIZE * sizeof(uint16_t));		// Allocate space for audio samples
	pInputBuffer = pInputBufferStart;														// Start buffer at beginning
	pInputBufferEnd = pInputBufferStart + AUDIO_BUFFER_SIZE;								// Save buffer end location
	pOutputBuffer = malloc(sizeof(uint16_t));

	pTunerResultPeriod = malloc(TUNER_RESULT_AVERAGES * sizeof(float32_t));
	pTunerResultPeriodStart = pTunerResultPeriod;
	pTunerResultPeriodEnd = pTunerResultPeriodStart + TUNER_RESULT_AVERAGES;

	pLooperBufferInstructionStart = malloc((LOOPER_BUFFER_SIZE + 2) * sizeof(uint16_t));
	pLooperBuffer = pLooperBufferInstructionStart + 2;
	pLooperBufferStart = pLooperBuffer;
	pLooperBufferEnd = pLooperBufferStart + LOOPER_BUFFER_SIZE;


	// Set up TIM1
	TIM1->CR1 |= (	TIM_CR1_ARPE 	|		// Enable auto-preload
					TIM_CR1_URS);			// Only overflow generates interrupt
	TIM1->CR2 |= 	TIM_CR2_CCPC;			// Enable CC1 preload
	TIM1->DIER |= (	TIM_DIER_UIE	|		// Generate interrupt on overflow
					TIM_DIER_CC1IE);		// Generate capture interrupt

	HAL_RCC_GetHCLKFreq();
	uint32_t timerCount = TIMER_FREQ_MHZ * 1000000 / FSAMPLE;
	TIM1->ARR = (uint16_t)timerCount;		// Calculate required timer value
	TIM1->CCR1 = (uint16_t)(timerCount / 2);// Generate capture at halfway through cycle
	TIM1->EGR |= TIM_EGR_UG;				// Generate update of registers
	TIM1->SR &= ~TIM_SR_UIF; 				// Clear update flag

	NVIC_SetPriority(TIM1_UP_TIM10_IRQn,4); // Set interrupt priority
	NVIC_EnableIRQ(TIM1_UP_TIM10_IRQn);		// Enable overflow interrupt

	NVIC_SetPriority(TIM1_CC_IRQn,4);		// Set interrupt priority
	NVIC_EnableIRQ(TIM1_CC_IRQn);			// Enable capture interrupt

	EXTI->IMR |= EXTI_IMR_MR2;				// Enable interrupt generated on EXTI2
	NVIC_SetPriority(EXTI2_IRQn,8);			// Set interrupt priority
	NVIC_EnableIRQ(EXTI2_IRQn);				// Enable EXTI2 interrupt

	Effects_Initialize(TIMER_FREQ_MHZ);		// Initialize Effects
}

void Audio_Enable(void)
{
	TIM1->CR1 |= TIM_CR1_CEN; // Start the timer
}

void Audio_Disable(void)
{
	TIM1->CR1 &= ~TIM_CR1_CEN; // Stop the timer
}


void Process_Audio(void)
{
	*pInputBuffer = ADC_Get_Result();	// Save the input signal in buffer
	switch(audioState)
	{
	// In default state, run input through effects and send it to output
	case AUDIO_STATE_DEFAULT:
		*pOutputBuffer = Apply_Effects(pInputBuffer);
		break;
	// In tuner state, send the input directly to the output if output flag is set high
	case AUDIO_STATE_TUNER:
		if(tunerOutput)
			*pOutputBuffer = *pInputBuffer;
		break;
	// In record state, apply effects to input then save them to looper buffer
	case AUDIO_STATE_RECORD:
		*pOutputBuffer = Apply_Effects(pInputBuffer);
		*pLooperBuffer = *pOutputBuffer;
		pLooperBuffer++;	// Increment buffer pointer
		if(pLooperBuffer >= pLooperBufferEnd)	// If end of buffer
		{
			pLooperBuffer = pLooperBufferStart;	// Wrap buffer pointer to beginning
			// Write the buffer to flash memory at current flash address
			FLASH_Page_Write_IT((uint8_t*)pLooperBufferInstructionStart, looperCurrentFlashAddress);
			looperCurrentFlashAddress += FLASH_PAGE_SIZE;	// Increment flash address
			// Check if too much data has been stored in flash
			if(looperCurrentFlashAddress > LOOPER_MAX_FLASH_ADDRESS)
			{
				Audio_Looper_Stop_Record();
			}
		}
		break;
	// In playback state, add the input data to the data from the looper
	case AUDIO_STATE_PLAYBACK:
		// Combine signals
		*pOutputBuffer = Apply_Effects(pInputBuffer) + *pLooperBuffer - DC_BIAS;
		pLooperBuffer++;	// Increment buffer pointer
		if(pLooperBuffer >= pLooperBufferEnd)	// If end of buffer
		{
			pLooperBuffer = pLooperBufferStart;	// Wrap buffer pointer to beginning
			// Read data from flash memory into the buffer
			FLASH_Page_Read_IT((uint8_t*)pLooperBufferInstructionStart, looperCurrentFlashAddress);
			looperCurrentFlashAddress += FLASH_PAGE_SIZE;	// Increment flash address
			// Loop flash address around if it overflows
			if(looperCurrentFlashAddress >= looperEndFlashAddress)
			{
				looperCurrentFlashAddress = LOOPER_STARTING_FLASH_ADDRESS;
			}
		}
		break;
	}

	pInputBuffer++;								// Increment buffer pointer
	if(pInputBuffer >= pInputBufferEnd)
	{
		pInputBuffer = pInputBufferStart;		// Wrap pointer around
		tunerReady = 1;							// Indicate that tuner can calculate value
	}
}

uint16_t* Audio_Get_Previous_Sample(uint16_t sampleNum)
{
	if(sampleNum > AUDIO_BUFFER_SIZE)
		return 0;
	uint16_t* sampleLoc = pInputBuffer - sampleNum;
	if(sampleLoc < pInputBufferStart)
	{
		sampleLoc+= AUDIO_BUFFER_SIZE;
	}
	return sampleLoc;
}

void Audio_Tuner_Activate(void)
{
	pInputBuffer = pInputBufferStart;
	pInputBufferEnd = pInputBufferStart + TUNER_BUFFER_SIZE;
	pTunerInputBuffer = (float32_t*) pInputBufferEnd + 1;
	pTunerResultBuffer = pTunerInputBuffer + TUNER_BUFFER_SIZE + 1;
	audioState = AUDIO_STATE_TUNER;
}

void Audio_Tuner_Deactivate(void)
{
	pInputBuffer = pInputBufferStart;
	pInputBufferEnd = pInputBufferStart + AUDIO_BUFFER_SIZE;
	audioState = AUDIO_STATE_DEFAULT;
}

void Audio_Tuner_Enable_Output(void)
{
	tunerOutput = 1;
}

void Audio_Tuner_Disable_Output(void)
{
	tunerOutput = 0;
}

uint8_t Audio_Tuner_Get_Output_Status(void)
{
	return tunerOutput;
}

uint8_t Audio_Tuner_Get_Ready_Flag(void)
{
	return tunerReady;
}
void Audio_Tuner_Reset_Ready_Flag(void)
{
	tunerReady = 0;
}

char* Audio_Tuner_Get_Note(uint8_t* note, int8_t* cents)
{
	*note = (uint8_t)round(detectedNote);
	*cents = (int8_t)((detectedNote - *note) * 100);
	if(*note != 0)
		return NOTE_NAMES[*note % 12 ];
	return "-";
}

uint16_t Audio_Tuner_Get_Reference_Freq(void)
{
	return tunerReferenceFreq;
}

void Audio_Tuner_Increase_Reference_Freq(void)
{
	if(tunerReferenceFreq < TUNER_REFERENCE_FREQ_MAX)
		tunerReferenceFreq++;
}

void Audio_Tuner_Decrease_Reference_Freq(void)
{
	if(tunerReferenceFreq > TUNER_REFERENCE_FREQ_MIN)
			tunerReferenceFreq--;
}

void Audio_Tuner(void)
{
	// Change buffer pointers to ensure they have their own memory space
	pTunerInputBuffer = (float32_t*) pInputBufferEnd + 1;
	pTunerResultBuffer = pTunerInputBuffer + TUNER_BUFFER_SIZE + 1;

	// Convert all samples in buffer to 32bit floats
	int i;
	for(i = 0; i < TUNER_BUFFER_SIZE; i++)
	{
		pTunerInputBuffer[i] = (float32_t)pInputBuffer[i];
	}

	// Take the mean of the samples to get the DC bias voltage
	float32_t dcBias;
	arm_mean_f32(pTunerInputBuffer, TUNER_BUFFER_SIZE, &dcBias);

	// Subtract the DC bias from each sample
	arm_offset_f32(pTunerInputBuffer, -1 * dcBias, pTunerInputBuffer, TUNER_BUFFER_SIZE);
	// Perform an autocorrelation function
	arm_correlate_f32(pTunerInputBuffer, TUNER_BUFFER_SIZE, pTunerInputBuffer,
						TUNER_BUFFER_SIZE, pTunerResultBuffer);
	// Autocorrelation result gives symmetric output, only want result starting from center
	pTunerResultBuffer += TUNER_BUFFER_SIZE - 1;

	// Initialize the terms of the m'(t) calculation to r'(0)
	float32_t mt_term1 = pTunerResultBuffer[0];
	float32_t mt_term2 = pTunerResultBuffer[0];

	uint8_t peakFinderState = 0;
	uint16_t temp;
	float32_t maxNSDF;
	float32_t maxNSDFPeriod;

	// Cycle through entire result buffer and calculate the NSDF incrementally
	for(i = 0; i < TUNER_BUFFER_SIZE; i++)
	{
		// Calculate the NSDF
		pTunerResultBuffer[i] = 2 * pTunerResultBuffer[i] / (mt_term1 + mt_term2);

		// Subtract the appropriate x^2 from terms of m'(t)
		mt_term1 -= pTunerInputBuffer[TUNER_BUFFER_SIZE - 1 - i] *
					pTunerInputBuffer[TUNER_BUFFER_SIZE - 1 - i];
		mt_term2 -= pTunerInputBuffer[i]*pTunerInputBuffer[i];

		// Algorithm to find peaks of NSDF
		switch(peakFinderState)
		{
		case 0:		// Detect first negative going zero crossing
			if(pTunerResultBuffer[i] < 0)
			{
				peakFinderState = 1;
				temp = i;
			}
			break;
		case 1:		// Detect first positive going zero crossing
			if(pTunerResultBuffer[i] > 0 && (i - temp) > PEAK_FINDER_BUFFER)
			{
				peakFinderState = 2;
				maxNSDF = pTunerResultBuffer[i];
				temp = i;
			}
			break;
		case 2: 	// Detect peak
			if(pTunerResultBuffer[i] > maxNSDF)
			{
				maxNSDF = pTunerResultBuffer[i];
				maxNSDFPeriod = i;
			}
			// Detect second negative going zero crossing
			if(pTunerResultBuffer[i] < 0 && (i - temp) > PEAK_FINDER_BUFFER &&
					maxNSDF > TUNER_NDSF_THRESHOLD)
			{
				peakFinderState = 3;
			}
			break;
		}
	}

	// Check to see if detected peak is a valid result
	if(maxNSDF > TUNER_NDSF_THRESHOLD && maxNSDFPeriod < 750 && maxNSDFPeriod > 50)
	{
		*pTunerResultPeriod = maxNSDFPeriod;	// Save the calculated period in buffer
		pTunerResultPeriod++;					// Increment buffer pointer
		// Wrap pointer around if overflow
		if(pTunerResultPeriod > pTunerResultPeriodEnd)
			pTunerResultPeriod = pTunerResultPeriodStart;

		// Use sorting algorithm to place previously recorded periods in order
		int i;
		int j;
		float32_t temp;
		float32_t sortedPeriod[TUNER_RESULT_AVERAGES];
		arm_copy_f32(pTunerResultPeriodStart, &sortedPeriod[0], TUNER_RESULT_AVERAGES);
		for(i = 0; i < TUNER_RESULT_AVERAGES - 1; i++)
		{
			for(j = i + 1; j < TUNER_RESULT_AVERAGES; j++)
			{
				if(sortedPeriod[j] < sortedPeriod[i])
				{
					temp = sortedPeriod[i];
					sortedPeriod[i] = sortedPeriod[j];
					sortedPeriod[j] = temp;
				}
			}
		}

		// Calculate the average period, using only the values around the median
		float32_t averagePeriod;
		arm_mean_f32(&sortedPeriod[0] + TUNER_RESULT_BUFFER,
						TUNER_RESULT_AVERAGES - TUNER_RESULT_BUFFER * 2, &averagePeriod);
		// Calculate the detected note on the MIDI scale
		detectedNote = log10(CALIBRATED_TUNER_SAMPLE_RATE * 16 /(averagePeriod *
								tunerReferenceFreq)) / NOTE_CALCULATION_DENOMINATOR;
	}
}

void Audio_Looper_Start_Record(void)
{
	if(looperEndFlashAddress == LOOPER_STARTING_FLASH_ADDRESS)
	{
		audioState = AUDIO_STATE_RECORD;
		pLooperBuffer = pLooperBufferStart;
	}
}

void Audio_Looper_Stop_Record(void)
{
	audioState = AUDIO_STATE_DEFAULT;
	looperEndFlashAddress = looperCurrentFlashAddress;
}

void Audio_Looper_Start_Playback(void)
{
	audioState = AUDIO_STATE_PLAYBACK;
	pLooperBuffer = pLooperBufferStart;
	looperCurrentFlashAddress = LOOPER_STARTING_FLASH_ADDRESS;
	FLASH_Page_Read_IT((uint8_t*)pLooperBufferInstructionStart, looperCurrentFlashAddress);
	looperCurrentFlashAddress += FLASH_PAGE_SIZE;
}

void Audio_Looper_Stop_Playback(void)
{
	audioState = AUDIO_STATE_DEFAULT;
}
void Audio_Looper_Delete(void)
{
	FLASH_Erase_Blocks_IT(0, LOOPER_MAX_FLASH_ADDRESS);
	looperCurrentFlashAddress = LOOPER_STARTING_FLASH_ADDRESS;
	looperEndFlashAddress = looperCurrentFlashAddress;
}

/*
 * Returns 0 if flash is empty, returns 1 if flash used
 */
uint8_t Audio_Looper_Get_Flash_Status(void)
{
	if(looperEndFlashAddress == 0)
		return 0;
	return 1;
}

uint8_t Audio_Get_State(void)
{
	return audioState;
}

void TIM1_UP_TIM10_IRQHandler(void)
{
	if(TIM1->SR & TIM_SR_UIF)
	{
		TIM1->SR &= ~TIM_SR_UIF;	// Reset flag
		ADC_Start_Conversion();
	}
}

void TIM1_CC_IRQHandler(void)
{
	if(TIM1->SR & TIM_SR_CC1IF)
	{
		TIM1->SR &= ~TIM_SR_CC1IF;	// Reset flag
		DAC_Write(pOutputBuffer);
	}
}

void EXTI2_IRQHandler(void)
{
	if(EXTI->PR & EXTI_PR_PR2)
	{
		EXTI->PR |= EXTI_PR_PR2; // Reset flag
		ADC_Reset_Complete_Flag();
		Process_Audio();
	}
}

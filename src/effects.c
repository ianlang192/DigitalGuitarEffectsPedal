/*
 * effects.c
 *
 *  Created on: May 2, 2018
 *      Author: ianla
 */

#include "effects.h"

#define MAX_NUM_PARAMS 10
#define MAX_NAME_LENGTH 20

#define TEMPO_MIN_VALUE_BPM			60
#define TEMPO_MAX_VALUE_BPM			300
#define TEMPO_DEFAULT_VALUE_BPM		120

#define TEMPO_MIN_PERIOD_MS			60000/TEMPO_MAX_VALUE_BPM
#define TEMPO_MAX_PERIOD_MS			60000/TEMPO_MIN_VALUE_BPM

#define TEMPO_NUM_AVERAGES		5


#define TIM3_PRESCALE	8000

static uint32_t TIMER_FREQ_MHZ;

typedef struct
{
	char name[MAX_NAME_LENGTH];
	uint8_t type;						// 0 for normal, 1 for tempo
	volatile uint16_t value;
	uint16_t minValue;
	uint16_t maxValue;
	uint16_t stepValue;
} Effect_Param_TypeDef;

typedef struct
{
	uint8_t number;
	char name[MAX_NAME_LENGTH];
	Effect_Param_TypeDef params[MAX_NUM_PARAMS];
	uint8_t numParams;
	uint16_t (* effect)(uint16_t* pInputData, Effect_Param_TypeDef* effectParams);
	volatile uint8_t activated;
} Effect_TypeDef;


static uint16_t distortion(uint16_t* pInputData, Effect_Param_TypeDef* effectParams);
static uint16_t echo(uint16_t* pInputData, Effect_Param_TypeDef* effectParams);
static uint16_t vibrato(uint16_t* pInputData, Effect_Param_TypeDef* effectParams);
static uint16_t flanger(uint16_t* pInputData, Effect_Param_TypeDef* effectParams);
static uint16_t chorus(uint16_t* pInputData, Effect_Param_TypeDef* effectParams);
static uint16_t reverb(uint16_t* pInputData, Effect_Param_TypeDef* effectParams);
static uint16_t delay(uint16_t* pInputData, Effect_Param_TypeDef* effectParams);

Effect_TypeDef effects[20];
uint8_t numEffects;
volatile uint8_t currentEffectNum = 0;
volatile uint8_t currentParamNum = 0;

volatile static uint16_t tempo = TEMPO_DEFAULT_VALUE_BPM;

void Effects_Initialize(uint32_t timerFreqMHz)
{
	TIMER_FREQ_MHZ = timerFreqMHz;
	TIM3->CR1 |= (	TIM_CR1_ARPE 	|		// Enable auto-preload
					TIM_CR1_OPM 	|		// Set one pulse mode
					TIM_CR1_URS);			// Only overflow generates interrupt
	TIM3->PSC = 	TIM3_PRESCALE;			// Prescale the timer
	TIM3->ARR = 0xFFFF;						// Set auto-reload to max
	TIM3->EGR |= TIM_EGR_UG;				// Generate update of registers
	TIM3->SR &= ~TIM_SR_UIF; 				// Clear update flag

	uint8_t i = 0;
	uint8_t j = 0;

	// Distortion
	effects[i].number = i + 1;
	strcpy(effects[i].name, "Distortion");
	effects[i].effect = distortion;
	effects[i].activated = 0;

	strcpy(effects[i].params[j].name, "Gain");
	effects[i].params[j].type = EFFECT_PARAM_TYPE_NORMAL;
	effects[i].params[j].value = 5;
	effects[i].params[j].minValue = 1;
	effects[i].params[j].maxValue = 10;
	effects[i].params[j].stepValue = 1;
	j++;
	strcpy(effects[i].params[j].name, "Boost");
	effects[i].params[j].type = EFFECT_PARAM_TYPE_NORMAL;
	effects[i].params[j].value = 2;
	effects[i].params[j].minValue = 1;
	effects[i].params[j].maxValue = 10;
	effects[i].params[j].stepValue = 1;
	effects[i].numParams = j + 1;
	i++;
	j = 0;


	// Echo
	effects[i].number = i + 1;
	strcpy(effects[i].name, "Echo");
	effects[i].effect = echo;
	effects[i].activated = 0;

	strcpy(effects[i].params[j].name, "Length (ms)");
	effects[i].params[j].type = EFFECT_PARAM_TYPE_NORMAL;
	effects[i].params[j].value = 200;
	effects[i].params[j].minValue = 10;
	effects[i].params[j].maxValue = 990;
	effects[i].params[j].stepValue = 10;
	j++;
	strcpy(effects[i].params[j].name, "Level");
	effects[i].params[j].type = EFFECT_PARAM_TYPE_NORMAL;
	effects[i].params[j].value = 50;
	effects[i].params[j].minValue = 1;
	effects[i].params[j].maxValue = 100;
	effects[i].params[j].stepValue = 1;
	effects[i].numParams = j + 1;
	i++;
	j = 0;

	// Vibrato
	effects[i].number = i + 1;
	strcpy(effects[i].name, "Vibrato");
	effects[i].effect = vibrato;
	effects[i].activated = 0;

	strcpy(effects[i].params[j].name, "Speed");
	effects[i].params[j].type = EFFECT_PARAM_TYPE_NORMAL;
	effects[i].params[j].value = 20;
	effects[i].params[j].minValue = 1;
	effects[i].params[j].maxValue = 100;
	effects[i].params[j].stepValue = 1;
	j++;
	strcpy(effects[i].params[j].name, "Depth");
	effects[i].params[j].type = EFFECT_PARAM_TYPE_NORMAL;
	effects[i].params[j].value = 50;
	effects[i].params[j].minValue = 1;
	effects[i].params[j].maxValue = 100;
	effects[i].params[j].stepValue = 1;
	effects[i].numParams = j + 1;
	i++;
	j = 0;

	// Flanger
	effects[i].number = i + 1;
	strcpy(effects[i].name, "Flanger");
	effects[i].effect = flanger;
	effects[i].activated = 0;

	strcpy(effects[i].params[j].name, "Speed");
	effects[i].params[j].type = EFFECT_PARAM_TYPE_NORMAL;
	effects[i].params[j].value = 50;
	effects[i].params[j].minValue = 1;
	effects[i].params[j].maxValue = 100;
	effects[i].params[j].stepValue = 1;
	j++;
	strcpy(effects[i].params[j].name, "Depth");
	effects[i].params[j].type = EFFECT_PARAM_TYPE_NORMAL;
	effects[i].params[j].value = 50;
	effects[i].params[j].minValue = 1;
	effects[i].params[j].maxValue = 100;
	effects[i].params[j].stepValue = 1;
	effects[i].numParams = j + 1;
	i++;
	j = 0;

	// Chorus
	effects[i].number = i + 1;
	strcpy(effects[i].name, "Chorus");
	effects[i].effect = chorus;
	effects[i].activated = 0;

	strcpy(effects[i].params[j].name, "Speed");
	effects[i].params[j].type = EFFECT_PARAM_TYPE_NORMAL;
	effects[i].params[j].value = 50;
	effects[i].params[j].minValue = 1;
	effects[i].params[j].maxValue = 100;
	effects[i].params[j].stepValue = 1;
	j++;
	strcpy(effects[i].params[j].name, "Depth");
	effects[i].params[j].type = EFFECT_PARAM_TYPE_NORMAL;
	effects[i].params[j].value = 50;
	effects[i].params[j].minValue = 1;
	effects[i].params[j].maxValue = 100;
	effects[i].params[j].stepValue = 1;
	effects[i].numParams = j + 1;
	i++;
	j = 0;

	// Delay
	effects[i].number = i + 1;
	strcpy(effects[i].name, "Delay");
	effects[i].effect = delay;
	effects[i].activated = 0;

	strcpy(effects[i].params[j].name, "Tempo");
	effects[i].params[j].type = EFFECT_PARAM_TYPE_TEMPO;
	effects[i].params[j].value = 0;
	effects[i].params[j].minValue = TEMPO_MIN_VALUE_BPM;
	effects[i].params[j].maxValue = TEMPO_MAX_VALUE_BPM;
	effects[i].params[j].stepValue = 1;
	j++;
	strcpy(effects[i].params[j].name, "Level");
	effects[i].params[j].type = EFFECT_PARAM_TYPE_NORMAL;
	effects[i].params[j].value = 50;
	effects[i].params[j].minValue = 1;
	effects[i].params[j].maxValue = 100;
	effects[i].params[j].stepValue = 1;
	effects[i].numParams = j + 1;
	i++;
	j = 0;
	numEffects = i;
}

char* Effect_Get_Current_Name(void)
{
	return effects[currentEffectNum].name;
}

uint8_t Effect_Get_Current_Number(void)
{
	return effects[currentEffectNum].number;
}

char* Effect_Param_Get_Current_Name(void)
{
	return effects[currentEffectNum].params[currentParamNum].name;
}
char* Effect_Param_Get_Previous_Name(void)
{
	if(currentParamNum != 0)
		return effects[currentEffectNum].params[currentParamNum - 1].name;
	return "";
}
char* Effect_Param_Get_Next_Name(void)
{
	if(currentParamNum != (effects[currentEffectNum].numParams - 1))
		return effects[currentEffectNum].params[currentParamNum + 1].name;
	return "";
}

uint16_t Effect_Param_Get_Current_Value(void)
{
	switch(effects[currentEffectNum].params[currentParamNum].type)
	{
	case EFFECT_PARAM_TYPE_NORMAL:
		return effects[currentEffectNum].params[currentParamNum].value;
		break;
	case EFFECT_PARAM_TYPE_TEMPO:
		return Effect_Get_Tempo();
		break;
	}
	return 0;
}
uint16_t Effect_Param_Get_Current_Max_Value(void)
{
	return effects[currentEffectNum].params[currentParamNum].maxValue;
}
uint16_t Effect_Param_Get_Current_Min_Value(void)
{
	return effects[currentEffectNum].params[currentParamNum].minValue;
}

void Effect_Previous(void)
{
	if(currentEffectNum != 0)
	{
		currentEffectNum--;
		currentParamNum = 0;
	}
}
void Effect_Next(void)
{
	if(currentEffectNum != (numEffects -1))
	{
		currentEffectNum++;
		currentParamNum = 0;
	}
}
void Effect_Param_Previous(void)
{
	if(currentParamNum != 0)
		currentParamNum--;
}

void Effect_Param_Next(void)
{
	if(currentParamNum != (effects[currentEffectNum].numParams - 1))
		currentParamNum++;
}

void Effect_Param_Increase_Current_Value(void)
{
	switch(effects[currentEffectNum].params[currentParamNum].type)
	{
	case EFFECT_PARAM_TYPE_NORMAL:
		if((effects[currentEffectNum].params[currentParamNum].value + effects[currentEffectNum].params[currentParamNum].stepValue) > effects[currentEffectNum].params[currentParamNum].maxValue)
					effects[currentEffectNum].params[currentParamNum].value = effects[currentEffectNum].params[currentParamNum].maxValue;
				else
					effects[currentEffectNum].params[currentParamNum].value += effects[currentEffectNum].params[currentParamNum].stepValue;
		break;
	case EFFECT_PARAM_TYPE_TEMPO:
		Effect_Increase_Tempo();
		break;
	}
}

void Effect_Param_Decrease_Current_Value(void)
{
	switch(effects[currentEffectNum].params[currentParamNum].type)
	{
	case EFFECT_PARAM_TYPE_NORMAL:
		if((effects[currentEffectNum].params[currentParamNum].minValue + effects[currentEffectNum].params[currentParamNum].stepValue) > effects[currentEffectNum].params[currentParamNum].value)
			effects[currentEffectNum].params[currentParamNum].value = effects[currentEffectNum].params[currentParamNum].minValue;
		else
			effects[currentEffectNum].params[currentParamNum].value -= effects[currentEffectNum].params[currentParamNum].stepValue;
		break;
	case EFFECT_PARAM_TYPE_TEMPO:
		Effect_Decrease_Tempo();
		break;
	}
}

void Effect_Activate_Current(void)
{
	effects[currentEffectNum].activated = 1;
}

void Effect_Deactivate_Current(void)
{
	effects[currentEffectNum].activated = 0;
}

uint8_t Effect_Get_Current_Activated_Status(void)
{
	return effects[currentEffectNum].activated;
}

void Effect_Calculate_Tempo(void)
{
	static uint16_t previousTimerPeriods[TEMPO_NUM_AVERAGES] = {0}; // Save previous values
	static uint8_t index = 0;

	// Get value from timer and convert to period in ms
	uint16_t timerPeriod =  (uint32_t)TIM3->CNT * TIM3_PRESCALE / (TIMER_FREQ_MHZ * 1000);
	TIM3->EGR |= TIM_EGR_UG;			// Reset timer
	TIM3->SR &= ~TIM_SR_UIF;			// Reset flag
	TIM3->CR1 |= TIM_CR1_CEN;			// Enable timer

	// Check if period is within limits
	if((timerPeriod < TEMPO_MIN_PERIOD_MS) || (timerPeriod > TEMPO_MAX_PERIOD_MS))
		return;

	// Save period in buffer
	previousTimerPeriods[index] = timerPeriod;
	index++;
	if(index >= TEMPO_NUM_AVERAGES)	// Wrap index around if overflow
		index = 0;

	// Calculate the average period from previous values
	uint8_t i;
	uint32_t averagePeriod = 0;
	for(i = 0; i < TEMPO_NUM_AVERAGES; i++)
	{
		averagePeriod += previousTimerPeriods[i];
	}
	averagePeriod = averagePeriod / TEMPO_NUM_AVERAGES;
	uint16_t newTempo = 60000 / averagePeriod;	// Calculate the tempo in BPM
	// Update tempo variable if valid
	if((newTempo > TEMPO_MIN_VALUE_BPM) && (newTempo < TEMPO_MAX_VALUE_BPM))
	{
		tempo = newTempo;
	}
}

uint16_t Effect_Get_Tempo(void)
{
	return tempo;
}


void Effect_Increase_Tempo(void)
{
	if(tempo + 1 <= TEMPO_MAX_VALUE_BPM)
		tempo++;
}

void Effect_Decrease_Tempo(void)
{
	if(tempo - 1 >= TEMPO_MIN_VALUE_BPM)
		tempo--;
}

uint16_t Apply_Effects(uint16_t* pInputData)
{
	// Cycle through available effects and call their corresponding function
	uint8_t i;
	uint16_t outputBuffer = *pInputData;
	for(i = 0; i < numEffects; i++)
	{
		if(effects[i].activated)
		{
			outputBuffer = effects[i].effect(&outputBuffer, &effects[i].params[0]);
		}
	}
	return outputBuffer;
}

static uint16_t distortion(uint16_t* pInputData, Effect_Param_TypeDef* effectParams)
{
	// Save parameters
	float32_t gain = effectParams[0].value;
	uint16_t boost = effectParams[1].value;

	// Get input value as float from -1 to 1
	float32_t x = (*pInputData - DC_BIAS)/(float32_t)33000;

	float32_t sign = (x > 0) - (x < 0); // Extract sign of number
	float32_t temp;
	arm_mult_f32(&x,&gain,&temp,1);	// Use FPU to multiply input by gain
	temp = temp*(-1)*sign*boost;
	float32_t accumulator = 1;		// Initialize accumulator
	float32_t currentProd = 1;

	// Use Taylor series approximation for exponential function
	int i = 0;
	for(i = 1; i <= 20; i++)
	{
		arm_mult_f32(&currentProd, &temp, &currentProd,1);	// Use FPU
		currentProd = currentProd/i;
		accumulator += currentProd;
	}

	float32_t outputData = sign*(1 - accumulator)/boost; // Calculate output data
	return (uint16_t)(outputData*33000 + DC_BIAS);	// Convert back to integer for output
}

static uint16_t echo(uint16_t* pInputData, Effect_Param_TypeDef* effectParams)
{
	// Calculate parameters
	uint32_t delay = (effectParams[0].value * FSAMPLE) / 1000; // Get delay in ms
	uint32_t currentDelay = delay;
	uint16_t decay = (uint32_t)effectParams[1].value;
	uint16_t outputData = *pInputData;
	uint16_t numEchos = 0;

	// Only calculate at max 10 echos to keep calculation short
	while(numEchos < 10)
	{
		// If echo length is too long for buffer, return from function
		if(currentDelay > AUDIO_BUFFER_SIZE)
			break;
		// Get delayed sample and add it to current sample
		uint16_t* pEcho = Audio_Get_Previous_Sample(currentDelay);
		outputData += (decay * (*pEcho - DC_BIAS)) / effectParams[1].maxValue;
		currentDelay += delay; 	// Increment delay to get next echo
		decay -= decay/4;		// Increase decay to make next echo quieter
		numEchos++;
	}
	return outputData;
}

// Param0 = speed
// Param1 = depth
static uint16_t vibrato(uint16_t* pInputData, Effect_Param_TypeDef* effectParams)
{
	static float32_t t = 0;	// Keep track of time variable for sine wave
	// Frequency ranges from 1 - 11Hz
	float32_t frequency = (float32_t)effectParams[0].value / 10 + 1;
	// Depth ranges from 0 - 2ms
	float32_t depth = (float32_t)effectParams[1].value * FSAMPLE / 50000;

	// Use FPU to calculate sine
	float32_t modulation = arm_sin_f32(2 * M_PI * frequency * t);
	// Calculate index using sine wave result
	uint16_t delay = (uint16_t)(1 + depth + depth * modulation);

	// Ensure argument of arm_sin_f32 is from 0 to 2pi
	t += 1/(float32_t)FSAMPLE;
	if(t >= 1/frequency)
		t = 0;

	// Only return the delayed signal
	return *Audio_Get_Previous_Sample(delay);
}

static uint16_t flanger(uint16_t* pInputData, Effect_Param_TypeDef* effectParams)
{
	static float32_t t = 0;	// Keep track of time variable for sine wave
	// Frequency ranges from 1 - 11Hz
	float32_t frequency = (float32_t)effectParams[0].value / 10 + 1;
	// Depth ranges from 0 - 2ms
	float32_t depth = (float32_t)effectParams[1].value * FSAMPLE / 50000;

	// Use FPU to calculate sine
	float32_t modulation = arm_sin_f32(2 * M_PI * frequency * t);

	// Calculate index using sine wave result
	uint16_t delay = (uint16_t)(1 + depth + depth * modulation);

	// Ensure argument of arm_sin_f32 is from 0 to 2pi
	t += 1/(float32_t)FSAMPLE;
	if(t >= 1/frequency)
		t = 0;

	// Return the current sample added to the delayed sample
	return *pInputData + *Audio_Get_Previous_Sample(delay) - DC_BIAS;
}

#define CHORUS_EFFECT_SCALE_FACTOR 2
static uint16_t chorus(uint16_t* pInputData, Effect_Param_TypeDef* effectParams)
{
	// Start each sine wave at a different point
	static float32_t t[5] = {0, 1/8.0, 1/6.0, 1/4.0, 1/2.0};
	// Frequency ranges from 1 - 3Hz
	float32_t frequency = (float32_t)effectParams[0].value / 50 + 1;
	// Depth ranges from 0 - 2ms
	float32_t depth = (float32_t)effectParams[1].value * FSAMPLE / 50000;

	int32_t outputData = *pInputData;

	// Cycle through different modulated delay lines
	uint8_t i;
	for(i = 0; i < 5; i++)
	{
		// Calculate delay from sine value
		float32_t modulation = arm_sin_f32(2 * M_PI * frequency * t[i]);
		uint16_t delay = (uint16_t)(1 + depth + depth * modulation);
		t[i] += 1/(float32_t)FSAMPLE;
		if(t[i] >= 1/(frequency))
			t[i] = 0;
		outputData += *Audio_Get_Previous_Sample(delay);
	}
	// Add all delayed samples together and divide by a scale factor to reduce volume
	return (uint16_t)((outputData - 6 * DC_BIAS) / CHORUS_EFFECT_SCALE_FACTOR + DC_BIAS);
}

#define REVERB_SCALE_FACTOR 256
static uint16_t reverb(uint16_t* pInputData, Effect_Param_TypeDef* effectParams)
{
	static uint16_t reverbIndex[14] = {40, 80, 120, 140, 160, 180, 200, 210, 220, 230, 240, 250, 260, 270};
	static uint16_t reverbCoeff[14] = {200, 180, 160, 150, 120, 110, 100, 90, 70, 60, 50, 40, 30, 20};
	uint16_t depth = effectParams[0].value;
	int32_t outputData = (int32_t)*pInputData * REVERB_SCALE_FACTOR;
	uint8_t i;
	for(i = 0; i < 14; i++)
	{
		outputData += ((int32_t)*Audio_Get_Previous_Sample(reverbIndex[i] * depth) - DC_BIAS) * reverbCoeff[i];
	}
	outputData = outputData / REVERB_SCALE_FACTOR;
	return (uint16_t)outputData;
}

static uint16_t delay(uint16_t* pInputData, Effect_Param_TypeDef* effectParams)
{
	uint16_t delay = (uint32_t)60 * FSAMPLE / tempo; 	// Get delay in ms
	uint16_t level = effectParams[1].value;				// Get level parameter
	// Calculate the delay value
	uint32_t delayValue = (uint32_t)*Audio_Get_Previous_Sample(delay) * level
								/ effectParams[1].maxValue - DC_BIAS;
	return *pInputData + delayValue;	// Add delayed sample to current sample
}

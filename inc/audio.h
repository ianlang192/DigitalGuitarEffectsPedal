/*
* * audio.h
 *
 *  Created on: Apr 5, 2018
 *      Author: Ian
 */

#ifndef AUDIO_H_
#define AUDIO_H_

#include "stm32F4xx.h"
#include "stm32f4xx_hal.h"
#include "DAC.h"
#include "ADC.h"
#include "FLASH.h"
#include "effects.h"
#include "malloc.h"

#define FSAMPLE 44100
#define AUDIO_BUFFER_SIZE 44100
#define TUNER_BUFFER_SIZE 1024
#define PEAK_FINDER_BUFFER 10 // Require 10 samples between zero crossings to count
#define TUNER_NDSF_THRESHOLD 0.2
#define TUNER_RESULT_AVERAGES 10
#define TUNER_RESULT_BUFFER 3	// Throw away 3 samples on either end
#define CALIBRATED_TUNER_SAMPLE_RATE 44100
#define LOOPER_BUFFER_SIZE 128
#define LOOPER_STARTING_FLASH_ADDRESS 0
#define LOOPER_MAX_FLASH_ADDRESS			0x200000			// Allocate 2MB for looper storage
#define DC_BIAS							32678

#define AUDIO_STATE_DEFAULT 	0
#define AUDIO_STATE_TUNER 		1
#define AUDIO_STATE_RECORD  	2
#define AUDIO_STATE_PLAYBACK 	3

void Audio_Init(uint32_t timerFreqMHz);
void Audio_Enable(void);
void Audio_Disable(void);
void Process_Audio(void);
void Audio_Tuner(void);
void Audio_Tuner_Activate(void);
void Audio_Tuner_Deactivate(void);
uint8_t Audio_Tuner_Get_Ready_Flag(void);
void Audio_Tuner_Reset_Ready_Flag(void);
char* Audio_Tuner_Get_Note(uint8_t* note, int8_t* cents);
uint16_t* Audio_Get_Previous_Sample(uint16_t sampleNum);
uint16_t Audio_Tuner_Get_Reference_Freq(void);
void Audio_Tuner_Increase_Reference_Freq(void);
void Audio_Tuner_Decrease_Reference_Freq(void);
void Audio_Tuner_Enable_Output(void);
void Audio_Tuner_Disable_Output(void);
uint8_t Audio_Tuner_Get_Output_Status(void);

void Audio_Looper_Start_Record(void);
void Audio_Looper_Stop_Record(void);
void Audio_Looper_Start_Playback(void);
void Audio_Looper_Stop_Playback(void);
void Audio_Looper_Delete(void);
uint8_t Audio_Looper_Get_Flash_Status(void);
uint8_t Audio_Get_State(void);

#endif /* AUDIO_H_ */

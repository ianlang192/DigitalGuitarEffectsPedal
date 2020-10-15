/*
 * effects.h
 *
 *  Created on: May 2, 2018
 *      Author: ianla
 */

#ifndef EFFECTS_H_
#define EFFECTS_H_

#include "stm32F4xx.h"
#include "stm32f4xx_hal.h"
#include "audio.h"
#include "string.h"

#define ARM_MATH_CM4
#include "arm_math.h"
#include "math.h"

#define EFFECT_PARAM_TYPE_NORMAL 	0
#define EFFECT_PARAM_TYPE_TEMPO 	1


void Effects_Initialize(uint32_t timerFreqMHz);
uint16_t Apply_Effects(uint16_t* pInputData);
char* Effect_Get_Current_Name(void);
uint8_t Effect_Get_Current_Number(void);
char* Effect_Param_Get_Current_Name(void);
char* Effect_Param_Get_Previous_Name(void);
char* Effect_Param_Get_Next_Name(void);
uint16_t Effect_Param_Get_Current_Value(void);
uint16_t Effect_Param_Get_Current_Max_Value(void);
uint16_t Effect_Param_Get_Current_Min_Value(void);
void Effect_Previous(void);
void Effect_Next(void);
void Effect_Param_Previous(void);
void Effect_Param_Next(void);
void Effect_Param_Increase_Current_Value(void);
void Effect_Param_Decrease_Current_Value(void);
void Effect_Activate_Current(void);
void Effect_Deactivate_Current(void);
uint8_t Effect_Get_Current_Activated_Status(void);
void Effect_Calculate_Tempo(void);
uint16_t Effect_Get_Tempo(void);
void Effect_Increase_Tempo(void);
void Effect_Decrease_Tempo(void);

#endif /* EFFECTS_H_ */

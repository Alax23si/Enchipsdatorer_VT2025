/*
 * abuzz.h
 *
 *  Created on: Apr 12, 2022
 *      Author: Alexander Göransson
 */

#ifndef INC_ABUZZ_H_
#define INC_ABUZZ_H_

#include "main.h"
extern TIM_HandleTypeDef htim1;

// Set up the values in TIM2 to give a nice beep. Very long period gets set.
void abuzz_start();

// Sets the signal to be of zero length.
void abuzz_stop();


// Sets the period to be long. 2.0 seconds between pulses.
void abuzz_p_long();

// Sets the period to be short. 0.3 seconds between pulses.
void abuzz_p_short();



void abuzz_start()
{
	TIM1->PSC 	= 40-1;
	TIM1->CNT   = 0x0000;
	TIM1->CCR2  = 20;
	HAL_GPIO_WritePin(abuzz_OP_GPIO_Port, abuzz_OP_Pin,GPIO_PIN_SET);
}



void abuzz_stop()
{
	TIM1->PSC  = 0x0000;
	TIM1->ARR  = 0xFFFF;
	TIM1->CCR2 = 0x0000;
	HAL_GPIO_WritePin(abuzz_OP_GPIO_Port, abuzz_OP_Pin,GPIO_PIN_RESET);

}

void abuzz_p_long()
{
	TIM1->ARR = 0x0F90;
	TIM1->CNT = 0x0000;
}


void abuzz_p_short()
{
	TIM1->ARR = 0x012b;
	TIM1->CNT = 0x0000;
}


#endif /* INC_ABUZZ_H_ */

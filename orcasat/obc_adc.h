/*
 * sfu_adc.h
 *
 *  Created on: Jun 4, 2018
 *      Author: Richard
 */

#ifndef SFUSAT_OBC_ADC_H_
#define SFUSAT_OBC_ADC_H_
#include "sys_common.h"

void sfuADCInit();
void adcUpdate();
uint16_t OBCCurrent();

#endif /* SFUSAT_OBC_ADC_H_ */

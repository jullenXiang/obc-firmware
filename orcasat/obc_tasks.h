/*
 * SFUTasks.h
 *
 *  Created on: Feb 17, 2017
 *      Author: Richard
 */

#ifndef SFUSAT_OBC_TASKS_H_
#define SFUSAT_OBC_TASKS_H_

#include "FreeRTOS.h"
#include "rtos_task.h"
#include "rtos_queue.h"

#include "gio.h"
#include "obc_hardwaredefs.h"
#include "obc_state.h"
#include "obc_task_main.h"
#include "obc_task_radio.h"
#include "obc_uart.h"
#include "sys_common.h"


// ------------ Task default priorities -------------------
// since we may create tasks all over the place, put default priorities here so it's easy to determine relative priority
#define MAIN_TASK_PRIORITY					2
#define BLINKY_TASK_DEFAULT_PRIORITY		3
#define SERIAL_TASK_DEFAULT_PRIORITY		5
#define RADIO_TASK_DEFAULT_PRIORITY			6
#define WATCHDOG_TASK_DEFAULT_PRIORITY		6
#define STATE_TASK_DEFAULT_PRIORITY			3
#define ADC_TASK_DEFAULT_PRIORITY			1
#define FLASH_TASK_DEFAULT_PRIORITY			4
#define FLASH_READ_DEFAULT_PRIORITY			3
#define FLASH_WRITE_DEFAULT_PRIORITY		4
#define TESTS_PRIORITY						3
#define LOGGING_TASK_DEFAULT_PRIORITY		3
#define STDTELEM_PRIORITY					4

extern TaskHandle_t xSerialTaskHandle;
extern QueueHandle_t xSerialTXQueue;
extern QueueHandle_t xSerialRXQueue;
void vSerialTask(void *pvParameters);

void blinky(void *pvParameters);
void vStateTask(void *pvParameters); // state checker

void vExternalTickleTask(void *pvParameters);
void vStdTelemTask(void *pvParameters);




#endif /* SFUSAT_OBC_TASKS_H_ */

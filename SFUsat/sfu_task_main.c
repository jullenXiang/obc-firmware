/*
 * sfu_task_main.c
 *
 *  Created on: May 23, 2017
 *      Authors: Steven and Richard
 */

#include "sfu_tasks.h"
#include "sfu_task_main.h"
#include "sfu_task_radio.h"
#include "sfu_scheduler.h"
#include "sfu_rtc.h"
#include "sfu_state.h"
#include "printf.h"
#include "sfusat_spiffs.h"
#include "sfu_fs_structure.h"
#include "flash_mibspi.h"
#include "sfu_startup.h"
#include "sun_sensor.h"
#include "sfu_i2c.h"
#include "stlm75.h"
#include "deployables.h"
#include "bq25703.h"
#include "sfu_adc.h"
#include "stdtelem.h"
#include "sfu_gps.h"

//  ---------- SFUSat Tests (optional) ----------
#include "sfu_triumf.h"
#include "unit_tests/unit_tests.h"
#include "examples/sfusat_examples.h"
#include "sfu_task_logging.h"
#include "obc_sci_dma.h"

// Perpetual tasks - these run all the time
TaskHandle_t xSerialTaskHandle = NULL;
TaskHandle_t xRadioTaskHandle = NULL;
TaskHandle_t xTickleTaskHandle = NULL;
TaskHandle_t xBlinkyTaskHandle = NULL;
TaskHandle_t xStateTaskHandle = NULL;
TaskHandle_t xFilesystemTaskHandle = NULL;

// Radio tasks
TaskHandle_t xRadioRXHandle = NULL;
TaskHandle_t xRadioTXHandle = NULL;
TaskHandle_t xRadioCHIMEHandle = NULL;
TaskHandle_t xLogToFileTaskHandle = NULL;

/* MainTask for all platforms except launchpad */
#if defined(PLATFORM_OBC_V0_5) || defined(PLATFORM_OBC_V0_4) || defined(PLATFORM_OBC_V0_3)
void vMainTask(void *pvParameters) {
	/**
	 * Hardware initialization
	 */

	serialInit();
	gioInit();
	xTaskCreate(vExternalTickleTask, "tickle", 128, NULL, WATCHDOG_TASK_DEFAULT_PRIORITY, &xTickleTaskHandle); // Start this right away so we don't reset
	uart_dma_init();

	sfuADCInit();
	spiInit();
	flash_mibspi_init();
	sfu_i2c_init();
	serialGPSInit();

	// ---------- SFUSat INIT ----------
	rtcInit();
    gio_interrupt_example_rtos_init();
	stateMachineInit(); // we start in SAFE mode

	// ---------- BRINGUP/PRELIMINARY PHASE ----------
	serialSendln("SFUSat Started!");
	printStartupType();

	// ---------- INIT RTOS FEATURES ----------
	// TODO: encapsulate these
	xSerialTXQueue = xQueueCreate(30, sizeof(portCHAR *));
	xSerialRXQueue = xQueueCreate(10, sizeof(portCHAR));
	xLoggingQueue = xQueueCreate(LOGGING_QUEUE_LENGTH, sizeof(LoggingQueueStructure_t));

	serialSendQ("created queue");
    //uart_dma_send("DMA UART test message #3 out of 3");
	// ---------- INIT TESTS ----------
	// TODO: if tests fail, actually do something
	// Also, we can't actually run some of these tests in the future. They erase the flash, for example
	uart_dma_test();
	test_adc_init();
	readGPS();
//	flash_erase_chip();

	setStateRTOS_mode(&state_persistent_data); // tell state machine we're in RTOS control so it can print correctly
	bms_test();

// --------------------------- SPIN UP TOP LEVEL TASKS ---------------------------
	xTaskCreate( blinky,  						// Function for the task to run
			"blinky", 							// Text name for the task. This is to facilitate debugging only.
			configMINIMAL_STACK_SIZE,  			// Stack depth - in words. So 4x this = bytes, 32x this = bits.
			NULL, 								// Task parameter lets you pass stuff in to the task BY REFERENCE. So watch that your data doesn't get deleted. Should probably use a queue instead.
			BLINKY_TASK_DEFAULT_PRIORITY,	  	// Priorities are in sfu_tasks.h
			&xBlinkyTaskHandle );				// Task handles are above

	//NOTE: Task priorities are #defined in sfu_tasks.h
	xTaskCreate(vSerialTask, "serial", 400, NULL, SERIAL_TASK_DEFAULT_PRIORITY, &xSerialTaskHandle);
	xTaskCreate(vStateTask, "state", 400, NULL, STATE_TASK_DEFAULT_PRIORITY, &xStateTaskHandle);
	xTaskCreate(vFilesystemLifecycleTask, "fs", 500, NULL, FLASH_TASK_DEFAULT_PRIORITY, &xFilesystemTaskHandle);
	xTaskCreate(vRadioTask, "radio", 600, NULL, portPRIVILEGE_BIT | 6, &xRadioTaskHandle);
	xTaskCreate(deploy_task, "deploy", 128, NULL, 4, &deployTaskHandle);

	/* Std telemetry */
	xTaskCreate(generalTelemTask, "t_gen", 300, NULL, 3, &xgeneralTelemTaskHandle);
	xTaskCreate(temperatureTelemTask, "t_temp", 700, NULL, 4, &xtemperatureTelemTaskHandle);
	xTaskCreate(transmitTelemUART, "t_send", 900, NULL, STDTELEM_PRIORITY, &xTransmitTelemTaskHandle);
	xTaskCreate(obcCurrentTelemTask, "t_curr", 900, NULL, 3, &xobcCurrentTelemTaskHandle);
	xTaskCreate(BMSTelemTask, "bms", 900, NULL, 3, &xBMSTelemTaskHandle);

 /* Startup things that need RTOS */  
 
	logPBISTFails();
	sfu_startup_logs();


// --------------------------- OTHER TESTING STUFF ---------------------------
	// Right when we spin up the main task, get the heap (example of a command we can issue)

	CMD_t test_cmd = {.cmd_id = CMD_GET, .subcmd_id = CMD_GET_HEAP};
	Event_t test_event = {.seconds_from_now = 3, .action = test_cmd};
	addEvent(test_event);

	// Example of scheduling a task
	test_event.seconds_from_now = 1;
	test_event.action.subcmd_id = CMD_GET_TASKS;
	addEvent(test_event);

//	CMD_t test_schd = {
//			.cmd_id = CMD_SCHED,
//			.subcmd_id = CMD_SCHED_ADD,
//			.cmd_sched_data = (CMD_SCHED_DATA_t){
//				.seconds_from_now = 8,
//						.scheduled_cmd_id = CMD_TASK,
//						.scheduled_subcmd_id = CMD_TASK_SUSPEND,
//						.scheduled_cmd_data = {
//								0x40,
//								0x00
//				}
//			}
//	};
//	addEventFromScheduledCommand(&test_schd);

	showActiveEvents();

	serialSendln("main tasks created");

	while (1) {
		CMD_t g;
		if (getAction(&g)) {
			char buffer[16] = {0};
			snprintf(buffer, 16, "%d:%d:%s", g.cmd_id, g.subcmd_id, g.cmd_data);
			checkAndRunCommand(&g);
			serialSendQ(buffer);
		}
		tempAddSecondToHET();
		vTaskDelay(pdMS_TO_TICKS(1000));
	}

}
#else

/* MainTask for launchpad
 * 	- launchpad doesn't usually have external HW connected
 * 	- running our HW init functions with non-existent hardware will often hang the system
 * 	- so skip certain HW initialization, task creation for HW-specific things
 * 	 */

void vMainTask(void *pvParameters) {
	/**
	 * Hardware initialization
	 */
	serialInit();
	gioInit();
	spiInit();
//	flash_mibspi_init();

	// ---------- SFUSat INIT ----------
	rtcInit();
    gio_interrupt_example_rtos_init();
	stateMachineInit(); // we start in SAFE mode

	// ---------- BRINGUP/PRELIMINARY PHASE ----------
	serialSendln("SFUSat Started!");
	printStartupType();

	// ---------- INIT RTOS FEATURES ----------
	// TODO: encapsulate these
	xSerialTXQueue = xQueueCreate(30, sizeof(portCHAR *));
	xSerialRXQueue = xQueueCreate(10, sizeof(portCHAR));
	xLoggingQueue = xQueueCreate(LOGGING_QUEUE_LENGTH, sizeof(LoggingQueueStructure_t));

	serialSendQ("created queue");

	// ---------- INIT TESTS ----------
	// TODO: if tests fail, actually do something
	// Also, we can't actually run some of these tests in the future. They erase the flash, for example

	// test_flash();
// 	test_triumf_init();
//	flash_erase_chip();

	setStateRTOS_mode(&state_persistent_data); // tell state machine we're in RTOS control so it can print correctly

// --------------------------- SPIN UP TOP LEVEL TASKS ---------------------------
	xTaskCreate( blinky,  						// Function for the task to run
			"blinky", 							// Text name for the task. This is to facilitate debugging only.
			configMINIMAL_STACK_SIZE,  			// Stack depth - in words. So 4x this = bytes, 32x this = bits.
			NULL, 								// Task parameter lets you pass stuff in to the task BY REFERENCE. So watch that your data doesn't get deleted. Should probably use a queue instead.
			BLINKY_TASK_DEFAULT_PRIORITY,	  	// Priorities are in sfu_tasks.h
			&xBlinkyTaskHandle );				// Task handles are above

	//NOTE: Task priorities are #defined in sfu_tasks.h
	xTaskCreate(vSerialTask, "serial", 300, NULL, SERIAL_TASK_DEFAULT_PRIORITY, &xSerialTaskHandle);
	xTaskCreate(vStateTask, "state", 400, NULL, STATE_TASK_DEFAULT_PRIORITY, &xStateTaskHandle);
//	xTaskCreate(sfu_fs_lifecycle, "fs life", 1500, NULL, 4, &xFSLifecycle);
	xTaskCreate(vLogToFileTask, "logging", 500, NULL, LOGGING_TASK_DEFAULT_PRIORITY, &xLogToFileTaskHandle);
//
//	xTaskCreate(vRadioTask, "radio", 300, NULL, RADIO_TASK_DEFAULT_PRIORITY, &xRadioTaskHandle);
//	vTaskSuspend(xRadioTaskHandle);
	xTaskCreate(vExternalTickleTask, "tickle", 128, NULL, WATCHDOG_TASK_DEFAULT_PRIORITY, &xTickleTaskHandle);

	// TODO: watchdog tickle tasks for internal and external WD. (Separate so we can hard reset ourselves via command, two different ways)
	// TODO: ADC task implemented properly with two sample groups
	// TODO: tasks take in the system state and maybe perform differently (ADC will definitely do this)


// --------------------------- OTHER TESTING STUFF ---------------------------
	// Right when we spin up the main task, get the heap (example of a command we can issue)
	CMD_t test_cmd = {.cmd_id = CMD_GET, .subcmd_id = CMD_GET_HEAP};
	Event_t test_event = {.seconds_from_now = 3, .action = test_cmd};
	addEvent(test_event);

	// Example of scheduling a task
	test_event.seconds_from_now = 1;
	test_event.action.subcmd_id = CMD_GET_TASKS;
	addEvent(test_event);

//	CMD_t test_schd = {
//			.cmd_id = CMD_SCHED,
//			.subcmd_id = CMD_SCHED_ADD,
//			.cmd_sched_data = (CMD_SCHED_DATA_t){
//				.seconds_from_now = 8,
//						.scheduled_cmd_id = CMD_TASK,
//						.scheduled_subcmd_id = CMD_TASK_SUSPEND,
//						.scheduled_cmd_data = {
//								0x40,
//								0x00
//				}
//			}
//	};
//	addEventFromScheduledCommand(&test_schd);

	showActiveEvents();

	serialSendln("main tasks created");

	while (1) {
		CMD_t g;
		if (getAction(&g)) {
			char buffer[16] = {0};
			snprintf(buffer, 16, "%d:%d:%s", g.cmd_id, g.subcmd_id, g.cmd_data);
			checkAndRunCommand(&g);
			serialSendQ(buffer);
		}
		tempAddSecondToHET();
		vTaskDelay(pdMS_TO_TICKS(1000));
	}

}
#endif

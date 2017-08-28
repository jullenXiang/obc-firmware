/*
 * These functions will be called in sys_startup so that we can preserve startup flags.
 * This allows us to determine, for example, that we restarted due to a watchdog or a reset button push.
 *
 * To get this working, we place #defines in the correct user code locations so that HALCoGen can regenerate the code without wiping our code out, and we can then rearrange chunks
 * of code that are originally generated by halcogen.
 *
 * In particular, we have to check the reset source AFTER running the RAM self tests, as these tests destroy everything in memory.
 */

#ifndef SFUSAT_SFU_STARTUP_H_
#define SFUSAT_SFU_STARTUP_H_

#include "sys_common.h"
#include "system.h"
#include "sys_selftest.h"


#define SFUSAT_STARTUP_METHOD 1 // if 1, we do our method.

// Macro setup to generate enum and array of strings for the following startup types:
// these are all reset triggers, allowing us to determine why the OBC was reset.
// https://stackoverflow.com/questions/9907160/how-to-convert-enum-names-to-string-in-c
#define FOREACH_STARTUP(startType) \
        startType(DEFAULT_START)   \
        startType(PORRST_START)  \
        startType(DEBUG_START)   \
        startType(SOFT_RESET_INTERNAL_START)  \
        startType(SOFT_RESET_EXTERNAL_START)  \
        startType(OSCFAIL_START)  \
        startType(WATCHDOG_START)  \
        startType(CPU_RESET_START)  \

#define GENERATE_ENUM(ENUM) ENUM,
#define GENERATE_STRING(STRING) #STRING,

typedef enum {
	FOREACH_STARTUP(GENERATE_ENUM)
} Start_t;

static const char *STARTUP_STRING[] = {
		FOREACH_STARTUP(GENERATE_STRING)
};

//
//typedef enum {
//	DEFAULT_START,
//	PORRST_START,
//	DEBUG_START,
//	SOFT_RESET_INTERNAL_START,
//	SOFT_RESET_EXTERNAL_START,
//	OSCFAIL_START,
//	WATCHDOG_START,
//	CPU_RESET_START
//} Start_t;


typedef struct Startup_Data{
	Start_t resetSrc;
} Startup_Data_t;




void startupInit(void);
void startupCheck(void); // this is the first default chunk of sys_startup generated by halcogen, but changed so we can recover the source of reset.
void printStartupType(void);

extern Startup_Data_t startData;

#endif

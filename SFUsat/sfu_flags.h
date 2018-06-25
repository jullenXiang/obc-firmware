/*
 * sfu_flags.h
 *
 *  Created on: Jun 23, 2018
 *      Author: Richard
 *
 *      - the primary aim of this is reusable flags that don't run into each other, not storage savings :D
 *
 *      Organization of the flag file:
 *      Data						Start Addr		End Addr		Notes
 *      "created +  timestamp"			0				19 max		added when created
 *      "PBIST_RESET:1"					24				?
 *      "FSYS_PREFIX:a"					48				?
 */

#ifndef SFUSAT_SFU_FLAGS_H_
#define SFUSAT_SFU_FLAGS_H_

#define RESET_FLAG_START 	14
#define RESET_FLAG_MSG		"PBIST_RESET:1"		/* when we reset once we log this */
#define RESET_CLEAR_MSG		"PBIST_RESET:0"
#define RESET_FLAG_LEN		strlen(RESET_FLAG_MSG) - 1

#define PREFIX_FLAG_START 	0
#define PREFIX_FLAG_BASE	"FSYS_PREFIX:a"		/* when we reset we record this */
#define PREFIX_FLAG_LEN		strlen(PREFIX_FLAG_BASE) - 1


#endif /* SFUSAT_SFU_FLAGS_H_ */

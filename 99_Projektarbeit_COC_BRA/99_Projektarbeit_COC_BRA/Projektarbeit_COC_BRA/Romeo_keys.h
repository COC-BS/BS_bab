/**@file*/

#ifndef _ROMEO_KEYS_h
#define _ROMEO_KEYS_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif
/**
 * @fn int getkey(void)
 * @brief read KEY1 to KEY3 of the miniQ on A6
 *
 * Read A6. Decide which key is pressed and increment that counter. On key release, evaluate the counters and return that code.
 * Please note: the code is returned only once on each key release.
 * This function is non-blocking. The function relies on a cyclic call with reasonable recurrence rate.
 * In case of low recurrence rates, you might want to decrease the ..PRESS constants.
 * This routine relies on the special configuration of three resistors connected in series, and 3 keys, each shunting a different connection
 * to ground. the numerical constants reflect the voltage levels that result of this configuration.
 * The routine can easily be adapted to other similar arrangements with more or less keys or with different resistor values.
 *
 * @param none
 *
 * @return int 1..3 for KEY1..KEY3, 4..6 for long press of KEY1..KEY3
 */
int getkey(void);

#endif


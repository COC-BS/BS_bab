/**@file*/
// 
// 
// 

#include "Romeo_keys.h"
/**
 * @fn int getkey(void)
 * @brief read keys S1 to S5 of the Romeo on A0 (or any freely selectable analog pin)
 *
 * Read A0. Decide which key is pressed and increment that counter. On key release, evaluate the counters and return that code.
 * Please note: the code is returned only once on each key release.
 * This function is non-blocking. The function relies on a cyclic call with reasonable recurrence rate.
 * In case of low recurrence rates, you might want to decrease the ..PRESS constants.
 * This routine relies on the special configuration of a set of resistors connected in series, and 5 keys, each shunting a different connection
 * to ground. the numerical constants reflect the voltage levels that result of this configuration.
 * The routine can easily be adapted to other similar arrangements with more or less keys or with different resistor values.
 *
 * @param none
 *
 * @return int 1..5 for S1..S5, 6..10 for long press of S1..S5
 */
#define S1_LIMIT 73 
#define S2_LIMIT 238 
#define S3_LIMIT 418 
#define S4_LIMIT 624 
#define S5_LIMIT 887 
#define ANALOG_CHANNEL 0

#define KEYPRESS 3
#define LONGPRESS 200
#define NOPRESS 2

#define NUMKEYS 5

/**
 * \brief reads the user input from 5 keys S1..S5 on a Romeo V2.2
 * 
 * For short press 1..5 is returned, for long press the numbers returned are 6..10
 * 
 * \return int   number of the key S1..S5 (see above)
 */
int getkey(void)
{
	static int keys[NUMKEYS+1];
	int data = 0;
	uint16_t val = analogRead(ANALOG_CHANNEL);

	if (val > 1020) { // no key pressed
		if (++keys[0] > NOPRESS) {
			for(uint8_t i = 0; i <= NUMKEYS; i++){
				if(keys[i] > KEYPRESS){
					data = i;
					if(keys[i] > LONGPRESS) data += 5;
				}
				keys[i] = 0;
			}
		}
		else data = 0;
	}
	else {
		if (val < S1_LIMIT && keys[1] < 32000) ++keys[1]; // key1 pressed
		else if (val < S2_LIMIT && keys[2] < 32000) ++keys[2]; //key2 pressed
		else if (val < S3_LIMIT && keys[3] < 32000) ++keys[3]; //key3 pressed
		else if (val < S4_LIMIT && keys[4] < 32000) ++keys[4]; //key4 pressed
		else if ( keys[5] < 32000) ++keys[5]; // key5 pressed
		data = 0;
	}
	return data;
}


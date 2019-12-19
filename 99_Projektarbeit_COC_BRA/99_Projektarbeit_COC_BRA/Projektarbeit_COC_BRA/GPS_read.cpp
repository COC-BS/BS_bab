// Author D. Binggeli
// 11.12.2019
// Read NMEA time
// -------------------------------------------------------------------------------
// This module demonstrates an easy way to the interpretation of NMEA data with
// the example of scanning the NMEA data stream for valid GPRMC frames, parsing the
// time data and converting it to make a "HH:MM:SS" display on an LCD.
// The NMEA data stream is also forwarded to the USB Serial interface, and vice versa.
// This helps to analyze the data with a NMEA tool like SIMcom GPS DEMO on the PC
// --------------------------------------------------------------------------------


/*
#include <Wire.h>
#include <hd44780.h>
#include <hd44780ioclass/hd44780_I2Cexp.h>
//Beginning of Auto generated function prototypes by Atmel Studio
void clearBuffer(void );
//End of Auto generated function prototypes by Atmel Studio

#define NMEA_TIME "GPRMC,"
#define TIMEZONE 1
#define BUFFERSIZE 64
char buffer[BUFFERSIZE];							// buffer for data from NMEA device
uint8_t count=0;									// counter for buffer array
char* ptr = NULL;
uint8_t frame = 0;



hd44780_I2Cexp lcd; // declare lcd object: auto locate & auto config expander chip
long t;
int hour, minute, second;
char s[12];

void setup()
{
	Serial1.begin(9600);								// the Serial1 baud rate
	Serial.begin(9600);									// the Serial port of Arduino baud rate.
	int status = lcd.begin(16, 2);
	if(status)
	{
		status = -status;								// convert negative status value to positive number
		hd44780::fatalError(status);					// does not return
	}
	// init was successful
	//// FHNW 2019 turn Backlight on is reversed in our LCD (on will turn off)
	//// after off, the display has to be activated in order to show characters
	lcd.off();
	lcd.display();
	lcd.print("Read NMEA time");						// Print a message to the LCD
}


void loop()
{
	while(Serial1.available()) {						// as long as data is available on NMEA device
		unsigned char c =  buffer[count] = Serial1.read();	// write data into array
		Serial.write(c);								// and write data to PC (Serial)
		if(count < BUFFERSIZE-1) count++;				// to avoid buffer overflow
		if(c == '$') {
			ptr = buffer;
			while(ptr < buffer + BUFFERSIZE)  *ptr++=0;			// fill with 0
			count = 0;								// start of frame found, reset buffer
			buffer[count++] = c;						// store start of frame
		}
		if(c == '*') {									// end of frame found, start conversion
			frame = 1;
			break;
		}
	}
	if(frame) {											// full frame in buffer, so parse and decode
		frame = 0;
		ptr = strstr(buffer, NMEA_TIME);				// scan for GPRMC keyword
		if(ptr != NULL) {								// GPRMC keyword found, read time
			ptr += strlen(NMEA_TIME);
			t = atol(ptr);								// parse time value into hour, minute, second
			second = t % 100;
			minute = (t / 100) % 100;
			hour = ((t / 10000) + TIMEZONE) % 24;
			lcd.setCursor(0, 1);						// print in "HH:MM:SS" form on lcd
			sprintf(s, "%.2d:%.2d:%.2d", hour, minute, second);
			lcd.print(s);
			ptr = buffer;
			while(ptr < buffer + BUFFERSIZE)  *ptr++=0;			// fill with 0
			count = 0;							// clear buffer and start new
		}
	}
	if (Serial.available()){							// if data is available from PC
		Serial1.write(Serial.read());					// write it to the NMEA device
	}
}
*/
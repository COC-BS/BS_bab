/*

#include <Arduino.h>

#include <Wire.h>
#include <hd44780.h>
#include <hd44780ioClass/hd44780_I2Cexp.h>

#include "Romeo_keys.h"


    Name:       Bewertetes Praktikum ebs 2019
    Created:	06.06.2019 08:11:55
    Author:     Cyrill Coray / Antonio Bruno


hd44780_I2Cexp lcd; //Declare LCD-Display

static int input = 0; //User input
int count = 0;

// The setup() function runs once each time the micro-controller starts
void setup()
{
	lcd.begin(16, 2); //Starts LCD-Display
	lcd.noBacklight();
	lcd.noCursor(); //Hides the LCD-Cursor
	lcd.home(); //Sets the cursor in the upper-left of the LCD
	lcd.print("Romeo V2.2");
	lcd.setCursor(0 , 1);
	lcd.print("Coray / Bruno");
	while (millis()<3000);
	lcd.clear();
	
	//pinMode(A0, INPUT);
}

// Add the main program code into the continuous loop() function
void loop()
{
	lcd.clear();
	//int key = analogRead(A0);
	input = getkey();
	lcd.print(input);
	lcd.setCursor(0,1);
	lcd.print(count);
	count++;
	delay(1000);
	
	
}
*/
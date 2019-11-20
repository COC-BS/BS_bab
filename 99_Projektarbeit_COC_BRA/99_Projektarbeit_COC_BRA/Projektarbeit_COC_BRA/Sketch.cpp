/**@file*/
#include <wire.h>
#include <hd44780.h>
#include <hd44780ioClass/hd44780_I2Cexp.h> // include i/o class header

#include "Romeo_keys.h"

hd44780_I2Cexp lcd;

class Zeit {
public:	
	int hh_;
	int mm_;
	int ss_;
	int set_;
public:
	Zeit(int h, int m, int s){ hh_=h; mm_=m; ss_=s; set_ =0;};
	int Change(int key);
	int Tick();
	int GetHours();
	int GetMinutes();
	int GetSeconds();
 };


class Zeit zeitLocal(17,15,0);
class Zeit zeit2(12,0,0); 
int status = 0, status2;
int buzzer = 0;
/**
 * @brief increments the time  by one second 
 *
 * @return returns 1 in case of a day overflow, 0 else
 */

int Zeit::Tick(void){
	int ret = 0;
	ss_ = (ss_ + 1) % 60; // Sekunden
	if (ss_ == 0) {
		mm_ = (mm_ + 1) % 60;  // Minuten
		if (mm_ == 0) {
			hh_ = (hh_ + 1) % 24; // Stunden
			if (hh_ == 0) ret = 1;
		}
	}
	return ret;
}

int Zeit::GetHours(){return hh_;};
int Zeit::GetMinutes(){return mm_;};
int Zeit::GetSeconds(){return ss_;};

/**
 * @brief A watch for the arduino, counting on the millis()
 *
 * requires a structure zeit_S with members ss,mm,hh,DD,MM,YYYY as int
 * frequent calls will result in counting the time. days-of-month and leap years are taken into account
 *
 * @return void
 */
void Watch()
{
#define INTERVAL 1000L
	static long target = INTERVAL;
	if (millis() > target)	{
		target += INTERVAL;
		zeitLocal.Tick();
		//if(zeit1.Tick()) datum1.Tick(); //Wenn die Zeit bei 24.00 ankommt wird ret = 1 gesetzt und das Datum eins weitergestellt
	}
#undef INTERVAL
}

#define NO_KEY 0
#define UP_KEY 1
#define LEFT_KEY 2
#define DOWN_KEY 3
#define RIGHT_KEY 4
#define OK_KEY 5
#define BK_KEY 6
#define X_KEY 10
/**
 * @brief print the given time in a "hh:mm:ss"-frame on a line of an lcd
 * 
 * requires the library LiquidCrystal or LiquidCrystal_I2C
 *
 * @param[in] zeit: struct 
 * 
 * @return void
 */
void printhhmmss(class Zeit &z)
{
	if(z.GetHours()<=9)lcd.print("0");
	lcd.print(z.GetHours());
	if(z.GetMinutes()<=9)lcd.print(":0"); else lcd.print(":");
	lcd.print(z.GetMinutes());
	if(z.GetSeconds()<=9)lcd.print(":0"); else lcd.print(":");
	lcd.print(z.GetSeconds());
}

/**
 * @brief print the humidity and temp on the lcd
 * @return void
 */
void printHumidityTemp (void)
{
	lcd.setCursor(0,1);
	lcd.print("Hum  &  Temp");
}

/**
 * @brief shows the locale time, the relative humidity and the temperature
 *
 * print a string "hh:mm:ss ALM" to the LCD
 * print the humidity and the temperature to the lower line
 *
 * @param[in] key : int, user input
 *
 * @return key
 */
int homeScreen(int key)
{
	lcd.setCursor(0,0);
	printhhmmss(zeitLocal);
	printHumidityTemp();
	
	return key;
}

int changeTimeZone (int key)
{
	switch (key)
	{
		case X_KEY:
		//zeit2 = zeitLocal;
		case DOWN_KEY:
		lcd.clear();
		zeit2.hh_-=1;
		key = X_KEY;
		break;
		case UP_KEY:
		lcd.clear();
		zeit2.hh_+=1;
		key = X_KEY;
		break;
		case OK_KEY:
		zeitLocal = zeit2;
		break;
		case RIGHT_KEY:
		zeitLocal = zeit2;
		break;
	}
	return key;
}

int timeZones(int key)
{
	int input = changeTimeZone(key);
	lcd.setCursor(0,1);
	printhhmmss(zeit2);
	return input;
}



typedef int (*PFA_T)(int key);
typedef int (*PFP_T)(void);
/**
 * @brief struct for one entry in the menu
 *
 *
 */
struct FSM_TAG
{
	//! text to print on the upper line of the lcd
	const char * text1;
	//! these fife members contain the number of the menu that is to be jumped to if the corresponding key is hit 
	int up, left, down, right, ok; 
	//! function active will be called as long as the menu is active
	PFA_T active; 
	 //! function goright will be called upon a hit on the 'right' key
	PFP_T goright;
	 //! function positive will be called upon a hit on the 'ok' key
	PFP_T positive;
};


const char str0[] PROGMEM = ""; //Keyword PROGMEM saves Data to the Flash-Memory
const char str1[] PROGMEM = "LOKALZEIT";
const char str2[] PROGMEM = "NEW YORK";
const char str3[] PROGMEM = "WAKE-UP TIME";
const char str4[] PROGMEM = "SET WAKE-UP TIME";
const char str5[] PROGMEM = "TIME";
const char str6[] PROGMEM = "SET TIME";
const char str7[] PROGMEM = "DATE";
const char str8[] PROGMEM = "SET DATE";

const struct FSM_TAG watchmenu[] =
{
	//            ^   <   v   >  ok
	/*0*/ {str0, -1, -1,  -1,  1, -1,	homeScreen,		NULL,		NULL},
	/*1*/ {str1, -1,  -1,  -1,  -1,   0,	timeZones,		NULL,		NULL},
		

};

static int menu = 0, newmenu = 0;
static int input = 0; /// user input
static char strtemp[20]; /// temporary variable for access to PROGMEM strings 
#define ROM(a) strcpy_P(strtemp, a) /// macro hiding the strcpy_P to the temp variable 'strtemp'
/**
 * \brief arduino setup
 * 
 * called once at program startup
 * \return void
 */
void setup()
{
	lcd.begin(16, 2);
	lcd.noBacklight();
	lcd.noCursor();
	lcd.home();
	lcd.print("Romeo V2.2 Watch");
	lcd.setCursor(0,1);
	lcd.print("Coray / Bruno");
	while(millis()<3000);
	lcd.clear();
}

/**
 * \brief arduino loop
 * called repeatedly in an endless loop
 * 
 * \return void
 */
void loop()
{
	Watch();
	input = getkey();
	if (watchmenu[menu].active) 
		input = watchmenu[menu].active(input);
		//! cyclic call to active function as long as menu n is active
	switch(input) {
		default:
		case X_KEY:
		case NO_KEY:
		newmenu = -1; // no changes made
		break;
		case UP_KEY:
		lcd.clear();
		newmenu = watchmenu[menu].up;
		break;
		case LEFT_KEY:
		lcd.clear();
		newmenu = watchmenu[menu].left;
		break;
		case DOWN_KEY:
		lcd.clear();
		newmenu = watchmenu[menu].down;
		break;
		case RIGHT_KEY:
		lcd.clear();
		if(watchmenu[menu].goright != NULL) {
			//! call go right function with new value
			watchmenu[menu].goright();
		}
		newmenu = watchmenu[menu].right;
		break;
		case OK_KEY:
		lcd.clear();
		if(watchmenu[menu].positive != NULL) {
			//! call positive answer function with new value
			watchmenu[menu].positive();
		}
		newmenu = watchmenu[menu].ok;
		break;
		case BK_KEY:
		lcd.clear();
		lcd.noCursor();
		lcd.noBlink();
		newmenu = 0;
		break;
	}
	if(newmenu >= 0){
		menu = newmenu;
		lcd.setCursor(0, 0);
		lcd.print(ROM(watchmenu[menu].text1));
	}
}

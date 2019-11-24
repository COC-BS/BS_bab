#include <wire.h>
#include <hd44780.h>
#include <hd44780ioClass/hd44780_I2Cexp.h> // include i/o class header

#include "Romeo_keys.h"

hd44780_I2Cexp lcd;

//Strukt um Städte und deren Zweitverschiebung zu GMT (London)
struct CITY_TIME_DIF {
	String name;
	int timediff;
	String initials;
};

const struct CITY_TIME_DIF CITIES [] {
	{"London", 0, "LOND"},{"New-York", -5, "NEYO"},{"Paris", 1, "PARI"},{ "Tokyo", 9, "TOKY"},
	{"Hongkong", 8, "HONG"},{"Los Angeles", -8, "LOSA"},{"Chicago", -6, "CHIC"},{"Seoul", 9, "SEOU"},
	{ "Bruessel",  1, "BRUE"},{"Washington",  -5, "WASH"},{"Singapur", 8, "SING"},{"Sydney", 11, "SYDN"}
	};

int tz = 0;
long readSensor = 0;
boolean timeZoneChoosen = false; //Bool to set the local time or a time of a city

//Pin deklaration für den Button und den Temperatur Sensor
const int btnPin = 7;
const int tempSensor=A4;

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

//Ausgangsuhrzeit
class Zeit zeitGMT(21,31,0);
//Zeit für die verschiedenen Zeitzonen
class Zeit zeitTimeZone(12,0,0);
//Lokalzeit
class Zeit zeitLocal(22,31,0);


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
		zeitGMT.Tick();
		zeitTimeZone.Tick();
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
 * 
 * Liest den Temperatursensor aus rechnet den Wert in Grad Celsius um
 * und schreibt den Wert auf das LCD
 *
 * @return void
 */
void printHumidityTemp (void)
{
	float temp;
	temp=analogRead(tempSensor);
	temp=(temp*500)/1023;
	
	lcd.setCursor(0,1);
	if (temp > 100) lcd.print("Sensor defekt");
	else {
		lcd.print(temp);
		lcd.print(" ");
		lcd.print((char)223);
		lcd.print("C");
	}
}

/**
 * @brief shows the locale time, the relative humidity and the temperature
 *
 * print a string "hh:mm:ss ALM" to the LCD
 * print the humidity and the temperature to the lower line every 5 seconds
 *
 * @param[in] key : int, user input
 *
 * @return key
 */
int homeScreen(int key)
{
	if (readSensor < millis())
	{	
		printHumidityTemp();
		readSensor = millis() + 5000;
	}
	lcd.setCursor(0,0);
	if (timeZoneChoosen)
	{
		printhhmmss(zeitTimeZone);
		lcd.setCursor(12,0);
		lcd.print(CITIES[tz].initials);
	}
	else
	{
		printhhmmss(zeitLocal);	
	}
	return key;
}

/**
 * @brief Errechnet die Zeit der Zeitzone
 * 
 * Stellt sicher, dass die Zeiten richtig sind. 
 * Zwischen 0 und kleiner als 24.
 */
void calculateTime() {
	zeitTimeZone.hh_= zeitGMT.hh_ + CITIES[tz].timediff;
	if (zeitTimeZone.hh_ > 23)
	{
		zeitTimeZone.hh_ -= 24; 
	}
	if (zeitTimeZone.hh_ < 0)
	{
		zeitTimeZone.hh_ = 24 - zeitTimeZone.hh_;
	}
	
}


/**
 * @brief Ermöglicht das durchwechseln verschiedener Zeitzonen
 * 
 * Input UP,DOWN Key wird verarbeitet und als X_KEY zurückgegeben 
 * Es wird die Zeit der Zeitzone mit calculateTime ermittelt
 * 
 * @return key
 */
int changeTimeZone (int key)
{
	switch (key)
	{
		case X_KEY:
		//zeit2 = zeitLocal;
		case DOWN_KEY:
		lcd.clear();
		if (tz == 0)
		{
			tz = 11;
		}
		else tz -= 1;
		calculateTime();	
		key = X_KEY;
		break;
		case UP_KEY:
		lcd.clear();
		if (tz == 11)
		{
			tz = 0;
		}
		else tz += 1;
		calculateTime();
		key = X_KEY;
		break;
		case OK_KEY:
		break;
		case RIGHT_KEY:
		break;
	}
	return key;
}

/**
 * @brief sets the choosen time Zone to the home screen
 *
 * @return 0
 */
int chooseTimeZone (void)
{
	timeZoneChoosen = true;
	return 0;
}

/**
 * @brief shows a city an his time
 *
 * change the city if a key is pressed
 * prints the name of the city an his time
 *
 * @param[in] key : int, user input
 *
 * @return key
 */
int setTimeZone(int key)
{
	int input = changeTimeZone(key);
	lcd.setCursor(0,0);
	lcd.print(CITIES[tz].name);
	lcd.setCursor(0,1);
	printhhmmss(zeitTimeZone);
	return input;
}


/**
 * @brief set zero position of the three pointer
 *
 */
void callibratePointer() 
{
	lcd.clear();
	lcd.print("Zeiger kalibrieren");
	delay(2000);
	lcd.clear();
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
	/*1*/ {str0, -1,  0,  -1,  -1, 0,	setTimeZone,		NULL,	chooseTimeZone},
		

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
	//LCD konfigurieren
	lcd.begin(16, 2);
	lcd.noBacklight();
	lcd.noCursor();
	lcd.home();
	lcd.print("Romeo V2.2 Watch");
	lcd.setCursor(0,1);
	lcd.print("Coray / Bruno");
	while(millis()<3000);
	lcd.clear();
	
	zeitTimeZone = zeitGMT;
	
	//Button uns Sensor Pin als input definiert
	pinMode(btnPin, INPUT);
	pinMode(tempSensor,INPUT);
	
}

/**
 * \brief arduino loop
 * called repeatedly in an endless loop
 * 
 * \return void
 */
void loop()
{
	//Button-Pin auslesen
	if (digitalRead(btnPin) == HIGH)
	{
		callibratePointer();
	}
	else
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
	} //else Klammer
}




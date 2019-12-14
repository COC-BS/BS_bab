#include <wire.h>
#include <hd44780.h>
#include <hd44780ioClass/hd44780_I2Cexp.h> // include i/o class header

#include <dht.h> //Library to read the temp and humidity sensor
#include "Romeo_keys.h"


hd44780_I2Cexp lcd;

//Strukt um Städte und deren Zweitverschiebung zu GMT (London)
struct CITY_TIME_DIF {
	String name;
	int timediff;
	String initials;
};

const struct CITY_TIME_DIF CITIES [] {
	{"London", 0, "LON"},{"New-York", -5, "NEY"},{"Paris", 1, "PAR"},{ "Tokyo", 9, "TOK"},
	{"Hongkong", 8, "HNK"},{"Los Angeles", -8, "LAN"},{"Chicago", -6, "CHI"},{"Seoul", 9, "SEO"},
	{ "Bruessel",  1, "BRU"},{"Washington",  -5, "WAS"},{"Singapur", 8, "SIN"},{"Sydney", 11, "SYD"}
	};

int tz = 0;
long readSensor = 0;
float temp;
float hum;
boolean timeZoneChoosen = false; //Bool to set the local time or a time of a city
boolean ampm = false; //Bool to change between 12h clock and 24h


//Pin deklaration für den Button und den Temperatur Sensor
const int btnPin = 7;
dht DHT; //Create a DHT object (Temp&Humidity Sensor)
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
 
 class Datum {
public:
	int YYYY_;
	int MM_;
	int DD_;
	int set_;
public:
	Datum(int d, int m, int y){DD_=d; MM_=m;YYYY_=y; set_ = 0;};
	int Change(int key);
	void Tick();
	int DaysOfMonth();
	int GetYear();
	int GetMonth();
	int GetDay();
};

//Ausgangsuhrzeit
class Zeit zeitGMT(4,50,0);
//Zeit für die verschiedenen Zeitzonen
class Zeit zeitTimeZone(12,0,0);
//Lokalzeit
class Zeit zeitLocal(5,50,0);

//Ausgangsdatum
class Datum datumGMT(14,12,2019);
//Datum für die verschiedenen Zeitzonen
class Datum datumTimeZone(1,9,2000);
//Lokales Datum
class Datum datumLocal(14,12,2019);



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
 * @brief increments the calendar date by one day 
 *
 * 
 * @return none
 */
void Datum::Tick(void){
	DD_ += 1;  // Tage
	if (DD_ > DaysOfMonth()) {
		DD_ = 1;
		MM_ += 1; // Monate
		if (MM_ > 12) {
			MM_ = 1;
			YYYY_ += 1; // Jahre
		}
	}
}

/**
 * @brief calculates the days of a month in a year
 *
 * returns 28,29,30 or 31 in accordance to month and year. every 4th year is considered a leap year. No correction dates
 * are taken into account
 *
 * @param[in] d struct Datum_S,
 * 
 * @return int, 28,29,30 or 31 in accordance to month and year
 */
int Datum::DaysOfMonth(void){
	switch (MM_) {
	case 2:
		if (YYYY_ % 4 != 0) return 28;
		else return 29;
	case 4: case 6: case 9: case 11:
		return 30;
	default:
		return 31;
	}
}
int Datum::GetYear(){return YYYY_;};
int Datum::GetMonth(){return MM_;};
int Datum::GetDay(){return DD_;};

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
		if(zeitGMT.Tick()) datumGMT.Tick(); //Wenn die Zeit bei 24.00 ankommt wird ret = 1 gesetzt und das Datum eins weitergestellt
		if(zeitTimeZone.Tick()) datumTimeZone.Tick();
		if(zeitLocal.Tick()) datumLocal.Tick(); 
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
 * Checks if the time format is 24h or am/pm and sets the format
 * requires the library LiquidCrystal or LiquidCrystal_I2C
 *
 * @param[in] zeit: struct 
 * 
 * @return void
 */
void printhhmmss(class Zeit &z)
{
	if (ampm)
	{
		int ampmhours = z.GetHours(); 
		if (ampmhours > 12)
		{
			 ampmhours -= 12;
			 if(ampmhours<=9)lcd.print("0");
			 lcd.print(ampmhours);
		}	
		else
		{
			if(ampmhours<=9)lcd.print("0");
			 lcd.print(ampmhours);
		}	 
	}
	
	else
	{
		if(z.GetHours()<=9)lcd.print("0");
		lcd.print(z.GetHours());
	}
	if(z.GetMinutes()<=9)lcd.print(":0"); else lcd.print(":");
	lcd.print(z.GetMinutes());
	if(z.GetSeconds()<=9)lcd.print(":0"); else lcd.print(":");
	lcd.print(z.GetSeconds());
	if (ampm && z.GetHours() > 12) lcd.print(" PM");
	else if (ampm && z.GetHours() <= 12) lcd.print(" AM");
}

/**
 * @brief print the given date in a "DD:MM:YYYY"-frame on a line of an lcd
 * 
 * requires the library LiquidCrystal or LiquidCrystal_I2C
 *
 * @param[in] datum: struct 
 * 
 * @return void
 */
void printddmmyyyy(class Datum &d)
{
	if(d.GetDay()<=9)lcd.print(" ");
	lcd.print(d.GetDay());
	if(d.GetMonth()<=9)lcd.print(". "); else lcd.print(".");
	lcd.print(d.GetMonth());
	lcd.print(".");
	lcd.print(d.GetYear());
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
	lcd.setCursor(0,1);
	if (temp == 0) lcd.print("Sensor defekt");
	else {
		lcd.print(temp);
		lcd.print((char)223);
		lcd.print("C   ");
		lcd.print(hum);
		lcd.print("%");
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
		int readData = DHT.read22(tempSensor);
		temp = DHT.temperature;
		hum = DHT.humidity;
		printHumidityTemp();
		readSensor = millis() + 5000;
	}
	printHumidityTemp();
	lcd.setCursor(0,0);
	if (timeZoneChoosen)
	{
		printhhmmss(zeitTimeZone);
		lcd.setCursor(13,0);
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
		//datumTimeZone.Tick();
	}
	if (zeitTimeZone.hh_ < 0)
	{
		zeitTimeZone.hh_ = 24 - zeitTimeZone.hh_;
		/*
		datumTimeZone.DD_ -= 1;
		if (datumTimeZone.DD_ = 0)
		{
			datumTimeZone.MM_ -= 1;
			if (datumTimeZone.MM_ != 0)
			{
				datumTimeZone.DD_ = datumTimeZone.DaysOfMonth();	
			}
			else
			{
				datumTimeZone.YYYY_ -= 1;
				datumTimeZone.DD_ = 31;
			}
		}
		*/
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
	timeZoneChoosen = !timeZoneChoosen;
	return 0;
}

/**
 * @brief change between 12h and 24h clock
 *
 * @return 0
 */
int changeAMPM (void)
{
	ampm = !ampm;
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


int dateAlarmScreen(int key)
{
	lcd.setCursor(0,0);
	if (timeZoneChoosen)
	{
		printddmmyyyy(datumTimeZone);
		lcd.setCursor(13,0);
		lcd.print(CITIES[tz].initials);
	}
	else
	{
		printddmmyyyy(datumLocal);
	}
	return key;
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
	//! these fife members contain the number of the menu that is to be jumped to if the corresponding key is hit 
	int up, left, down, right, ok; 
	//! function active will be called as long as the menu is active
	PFA_T active; 
	 //! function goright will be called upon a hit on the 'right' key
	PFP_T goright;
	 //! function positive will be called upon a hit on the 'ok' key
	PFP_T positive;
};


const struct FSM_TAG watchmenu[] =
{
	//      ^   <    v    >  ok
	/*0*/ {-1, -1,   2,   1,  0,	homeScreen,			NULL,		changeAMPM},
	/*1*/ {-1,  0,  -1,  -1,  0,	setTimeZone,		NULL,	chooseTimeZone},
	/*2*/ {0,  -1,  -1,  -1, -1,	dateAlarmScreen,	NULL,	NULL},		
		

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
	datumTimeZone = datumGMT;
	
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
	/*
	if (digitalRead(btnPin) == HIGH)
	{
		callibratePointer();
	}
	else
	{
	*/
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
	if(newmenu >= 0) menu = newmenu;

	//} //else Klammer
}




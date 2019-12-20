#include <wire.h>
#include <hd44780.h>
#include <hd44780ioClass/hd44780_I2Cexp.h>

#include <dht.h> //Library für den DHT22-Sensor
#include "Romeo_keys.h" //Library um die 5 Tasten auf dem Romeo auszuwerten
#include <PID_v1.h>


hd44780_I2Cexp lcd;

//Strukt um Städte und deren Zweitverschiebung zu GMT (London) abzuspeichern
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

int tz = 0; //Variable um durch das "CITIES"-Struct zu wechseln

//Sensorobjekt und Variabeln zur Sensorauswertung
dht DHT;
long readSensor = 0; 
float temp;
float hum;

boolean timeZoneChoosen = false; //Boolean um die Lokalzeit oder die Zeit einer Stadt anzuzeigen
boolean ampm = false; //Boolean um zwischen 24h und am/pm Layout zu wechseln


//Pin deklaration für den Button und den Temperatur Sensor
const int btnPin = 8;
const int tempSensor=A4;

//Pins und Variablen für den DC-Motor und seine PID-Regelung
const byte encoder0pinB = 7;
int En_Motor = 5; //Enable Control Motor 1
int Dir_Motor = 4; //Direction control Motor 1
byte encoder0PinALast;
double duration,abs_duration;//the number of the pulses
boolean result;
boolean motorOn = false;

double val_output;//Power supplied to the motor PWM value.
double Setpoint;
double Kp=3, Ki=5, Kd=0;
PID myPID(&abs_duration, &val_output, &Setpoint, Kp, Ki, Kd, DIRECT);

// Counters for milliseconds during interval
long previousMillis = 0;
long currentMillis = 0;

// One-second interval for measurements
int interval = 1000;

//GPS-Variabeln
#define NMEA_TIME "GPRMC,"
#define TIMEZONE 1
#define BUFFERSIZE 64
char buffer[BUFFERSIZE];							// buffer for data from NMEA device
uint8_t count=0;									// counter for buffer array
char* ptr = NULL;
uint8_t frame = 0;
long t;
char s[12];
//Flag um festzuhalten ob GPS Daten empfangen wurden und wann.
boolean gpsDataReceived = false;
int timeDataReceived;


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
class Zeit zeitGMT(16,59,45);
//Zeit für die verschiedenen Zeitzonen
class Zeit zeitTimeZone(0,0,0);
//Lokalzeit
class Zeit zeitLocal(17,59,45);

class Zeit weckzeit(18,0,0);
boolean weckerStatus = false;
int buzzer = 0;

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
 * @brief modifies the struct zeit 
 *
 * on each call, exactly one user input is processed 
 *
 * @param[in] key: int, user input
 *
 * input UP, DOWN, RIGHT, LEFT_KEY is processed and returned as NO_KEY 
 *
 * @return int processed user input,  all except the four above mentioned is returned
 */
int Zeit::Change(int key)
{
	if((set_ < 1) || (set_ > 2)) set_ = 1;
	switch(key){
	case RIGHT_KEY:
		set_ = 2;
		key = X_KEY;
		break;
	case LEFT_KEY:
		set_ = 1;
		key = X_KEY;
		break;
	}
	
	if(set_ == 1){
		//! change hours
		switch(key){
		case UP_KEY:
			hh_=(hh_+1)%24;
			ss_ = 0;
			key=X_KEY;
			break;
		case DOWN_KEY:
			if(hh_==0) hh_=23;
			else --hh_;
			ss_ = 0;
			key=X_KEY;
			break;
		}
	}
	else if(set_ == 2){
		//! change minutes
		switch(key){
		case UP_KEY:
			mm_=(mm_+1)%60;
			ss_ = 0;
			key=X_KEY;
			break;
		case DOWN_KEY:
			if(mm_==0) mm_=59;
			else --mm_;
			ss_ = 0;
			key=X_KEY;
			break;
		}
	}
	return key;
}

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
 * Ändert das Datum bei Zeitübertrag.
 */
void calculateTime() 
{
	zeitTimeZone = zeitGMT;
	zeitTimeZone.hh_= zeitGMT.hh_ + CITIES[tz].timediff;
	datumTimeZone = datumGMT;
	if (zeitTimeZone.hh_ > 23)
	{
		zeitTimeZone.hh_ -= 24;
		datumTimeZone.Tick();
	}
	if (zeitTimeZone.hh_ < 0)
	{
		zeitTimeZone.hh_ = 24 + zeitTimeZone.hh_;
		datumTimeZone.DD_ -= 1;
		if (datumTimeZone.DD_ == 0)
		{
			datumTimeZone.MM_ -= 1;
			if (datumTimeZone.MM_ != 0)	datumTimeZone.DD_ = datumTimeZone.DaysOfMonth();	
			else
			{
				datumTimeZone.YYYY_ -= 1;
				datumTimeZone.DD_ = 31;
			}
		}
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
	//lcd.setCursor(11,0);
	//printddmmyyyy(datumTimeZone);
	return input;
}

/**
 * @brief Zeigt den Weckerstatus und die Weckzeit
 *
 * Zeigt den Weckerstatus und die Weckzeit auf dem LCD	
 *
 * @param[in] key : int, user input
 *
 * @return key
 */
int alarmScreen(int key)
{
	lcd.setCursor(0,0);
	lcd.print("Wecker: ");
	if (weckerStatus)
	{
		lcd.print("Ein");
		
	}
	else lcd.print("Aus");
	lcd.setCursor(0,1);
	printhhmmss(weckzeit);
	return key;
}

/**
 * @brief Anzeige um die Weckzeit zu ändern
 *
 * Zeigt die Weckzeit auf dem LCD
 * Durch die Taster auf dem Romeo kann die Weckzeit geändert werden	
 *
 * @param[in] key : int, user input
 *
 * @return key
 */
int changeAlarm (int key)
{
	lcd.setCursor(0,0);
	lcd.print("Weckzeit?");
	key = weckzeit.Change(key);
	lcd.setCursor(0,1);
	printhhmmss(weckzeit);

		return key;
	
	return key;
}

/**
 * @brief Schaltet den Wecker ein oder aus
 *
 * @return 0
 */
int setAlarm (void)
{
	weckerStatus = !weckerStatus;
	return 0;
}

/**
 * @brief Zeigt Zeit und Datum auf dem LCD
 *
 * Zeigt je nach Einstellungen die Lokalzeit und das Lokaldatum
 * oder die Zeit, den Kürzel und das Datum einer Stadt an
 
 * @param[in] key : int, user input
 *
 * @return key
 */
int dateScreen(int key)
{
	lcd.setCursor(0,0);
	if (timeZoneChoosen)
	{
		printhhmmss(zeitTimeZone);
		lcd.setCursor(13,0);
		lcd.print(CITIES[tz].initials);
		lcd.setCursor(0,1);
		printddmmyyyy(datumTimeZone);
	}
	else
	{
		printhhmmss(zeitLocal);
		lcd.setCursor(0,1);
		printddmmyyyy(datumLocal);
	}
	return key;
}

/**
 * @brief Zeigt die GPS-DAten an
 *
 *
 *
 * @param[in] key : int, user input
 *
 * @return key
 */
int gpsScreen(int key)
{
	lcd.setCursor(0,0);
	lcd.print("GPS Daten");
	return key;	
}


/**
 * @brief set zero position of the three pointer
 *
 */
void callibratePointer() 
{	
	analogWrite(En_Motor,0);
	lcd.clear();
	lcd.print("Zeiger kalibrieren");
	delay(2000);
	lcd.clear();
}

void advance()//Motor Forward
{
	digitalWrite(Dir_Motor,LOW);
	analogWrite(En_Motor,val_output);
}

/**
 * @brief ISR um den Interrupt durch den Encoder zu bearbeiten
 *
 */
void wheelSpeed()
{
	duration++;
}

boolean readGPS()
{
	boolean getData = false;
	while(Serial1.available()) {							// as long as data is available on NMEA device
		unsigned char c =  buffer[count] = Serial1.read();	// write data into array
		Serial.write(c);									// and write data to PC (Serial)
		if(count < BUFFERSIZE-1) count++;					// to avoid buffer overflow
		if(c == '$') {
			ptr = buffer;
			while(ptr < buffer + BUFFERSIZE)  *ptr++=0;		// fill with 0
			count = 0;										// start of frame found, reset buffer
			buffer[count++] = c;							// store start of frame
		}
		if(c == '*') {										// end of frame found, start conversion
			frame = 1;
			break;
		}
	}
	if(frame) {												// full frame in buffer, so parse and decode
		frame = 0;
		ptr = strstr(buffer, NMEA_TIME);					// scan for GPRMC keyword
		if(ptr != NULL) {									// GPRMC keyword found, read time
			ptr += strlen(NMEA_TIME);
			t = atol(ptr);									// parse time value into hour, minute, second
			zeitLocal.ss_ = t % 100;
			zeitLocal.mm_ = (t / 100) % 100;
			zeitLocal.hh_ = ((t / 10000) + TIMEZONE) % 24;	
			
			getData = true;
			
			ptr += 10;
			//Readout Latitude & Longitude
			String latitude;
			//strlcpy(latitude, buffer + ptr,9);
			
			ptr += 9; 
			String longitude;
			//strlcpy(longitude, buffer + ptr, 9);
								
			ptr = buffer;
			while(ptr < buffer + BUFFERSIZE)  *ptr++=0;		// fill with 0
			count = 0;										// clear buffer and start new
		}
	}
	if (Serial.available()){								// if data is available from PC
		Serial1.write(Serial.read());						// write it to the NMEA device
	}
	zeitGMT = zeitLocal;
	zeitGMT.hh_ -= 1;
	return getData;
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
	//      ^   <    v    >  ok		active		hit 'right'   hit 'ok'
	/*0*/ {1,  -1,   3,   2,  0,	homeScreen,		NULL,	changeAMPM},
	/*1*/ {5,  -1,   0,   2,  1,	dateScreen,		NULL,	changeAMPM},		
	/*2*/ {-1,  0,  -1,  -1,  0,	setTimeZone,	NULL,	chooseTimeZone},
	/*3*/ {0,  -1,   5,   4, -1,	alarmScreen,	NULL,	setAlarm},
	/*4*/ {4,   3,   4,  -1,  3,	changeAlarm,	NULL,	NULL},	
	/*5*/ {3,   3,   0,  -1,  3,	gpsScreen,		NULL,	NULL},		
		
		

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
	Serial1.begin(9600);
	Serial.begin(9600);
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
	
	//Pin definitionen
	pinMode(btnPin, INPUT);
	pinMode(tempSensor,INPUT);
	pinMode(Dir_Motor, OUTPUT);
	pinMode(En_Motor, OUTPUT);
	pinMode(encoder0pinB,INPUT);
	
	//PID-Regler
	Setpoint = 20; //Setpint 15 works
	myPID.SetMode(AUTOMATIC);//PID is set to automatic mode
	myPID.SetSampleTime(100);//Set PID sampling frequency is 100ms
	attachInterrupt(digitalPinToInterrupt(7), wheelSpeed, CHANGE); //Pin 7 -> Interrupt 4
	previousMillis = millis();
	
	//GPS-Modul auslesen
	while (millis()<6000)
	{
		lcd.setCursor(0,0);
		lcd.print("Read GPS");	
		if (readGPS())
		{
			lcd.clear();
			lcd.print("Data received");
			gpsDataReceived = true;
			timeDataReceived = millis();
			break;
		}
		else timeDataReceived = millis();
	}
	if (gpsDataReceived == false) 
	{
		lcd.clear();
		lcd.print("No GPS-Signal");	
	}
	zeitTimeZone = zeitGMT;
	while((timeDataReceived+2000) > millis());
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
	//Wenn keine GPS-Daten vorhanden sind wird weiter nach Daten gesucht.
	//Wurden beim Setup GPS-DAten gefunden wird alle 30min die Uhrzeit und das Datum synchronisiert.
	if (gpsDataReceived == false || (timeDataReceived + 1000*60*30) < millis()) 
	{
		gpsDataReceived = readGPS();
		timeDataReceived = millis();
	}	
		
	//Button-Pin auslesen und auf Nullstellung reagieren
	if (digitalRead(btnPin) == HIGH)
	{
		Setpoint = 0;
	}
	else
	{
	//Aktueller Zeigerstand als Nullstellung definiert
	if (zeitLocal.GetSeconds() == 0) Setpoint = 20;
	//PID-Regelung
	advance(); //Motor forward
	currentMillis = millis();
	if (currentMillis - previousMillis > interval)
	{
		previousMillis = currentMillis;
		
		abs_duration=duration * 60 / 1920;
			
		result=myPID.Compute();//PID conversion is complete and returns 1
		if(result)
		{
			duration = 0; //Count clear, wait for the next count
		}
	}
	Watch();
	if(weckerStatus&&(zeitLocal.GetHours()==weckzeit.GetHours())&&(zeitLocal.GetMinutes()==weckzeit.GetMinutes())&&(zeitLocal.GetSeconds()==weckzeit.GetSeconds())) buzzer = 1;
	input = getkey();
		if(buzzer)
		{
			if(input){
				lcd.noBacklight();
				buzzer = 0;
			}
			else if(zeitLocal.GetSeconds()%2) lcd.backlight(); else lcd.noBacklight();
		}
	if (watchmenu[menu].active) input = watchmenu[menu].active(input);
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

	} //else Klammer
}




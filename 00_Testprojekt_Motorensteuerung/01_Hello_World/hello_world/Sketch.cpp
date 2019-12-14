//The sample code for driving one way motor encoder
#include <Arduino.h>
#include <Wire.h>
#include <PID_v1.h>
#include <hd44780.h>
#include <hd44780ioClass/hd44780_I2Cexp.h> // include i/o class header


/*
hd44780_I2Cexp lcd;

// Motor encoder output pulse per rotation (change as required)
#define ENC_COUNT_REV 1920

// Encoder output to Arduino Interrupt pin
#define ENC_IN 7

// MD10C PWM connected to pin 10
#define PWM 5
// MD10C DIR connected to pin 12
#define DIR 4

// Pulse count from encoder
volatile long encoderValue = 0;

// One-second interval for measurements
int interval = 1000;

// Counters for milliseconds during interval
long previousMillis = 0;
long currentMillis = 0;

// Variable for RPM measuerment
int rpm = 0;

// Variable for PWM motor speed output
int motorPwm = 0;

void updateEncoder()
{
	// Increment value for each pulse from encoder
	encoderValue++;
}

void setup()
{
	// Setup Serial Monitor
	lcd.begin(16,2);
	lcd.noBacklight();
	
	// Set encoder as input with internal pullup
	pinMode(ENC_IN, INPUT_PULLUP);
	
	// Set PWM and DIR connections as outputs
	pinMode(PWM, OUTPUT);
	pinMode(DIR, OUTPUT);
	
	// Attach interrupt
	attachInterrupt(digitalPinToInterrupt(ENC_IN), updateEncoder, CHANGE);
	
	// Setup initial values for timer
	previousMillis = millis();
}

void loop()
{
	
	// Control motor with potentiometer
	motorPwm = 0; //0-255
	
	// Write PWM to controller
	analogWrite(PWM, motorPwm);
	
	// Update RPM value every second
	currentMillis = millis();
	if (currentMillis - previousMillis > interval) {
		previousMillis = currentMillis;
		
		
		// Calculate RPM
		rpm = (float)(encoderValue * 60 / ENC_COUNT_REV);
		
		// Only update display when there is a reading
		if (motorPwm > 0 || rpm > 0) {
			lcd.clear();
			//lcd.print(encoderValue);
			lcd.print(rpm);
			lcd.print(" RPM");
			lcd.setCursor(0,1);
			lcd.print(encoderValue);
		}
		
		encoderValue = 0;
	}
}
*/

//-------------------------------------------------------------
//-------------- Motor PID -----------------------------
//-------------------------------------------------------------

/*
const byte encoder0pinA = 6;//A pin 
const byte encoder0pinB = 7;//B pin 
int E_left =5; //Enable Control Motor 1
int M_left =4; //Direction control Motor 1
byte encoder0PinALast;
double duration,abs_duration;//the number of the pulses
boolean Direction;//the rotation direction
boolean result;

double val_output;//Power supplied to the motor PWM value.
double Setpoint;
double Kp=65, Ki=5, Kd=0;
PID myPID(&abs_duration, &val_output, &Setpoint, Kp, Ki, Kd, DIRECT);

// Counters for milliseconds during interval
long previousMillis = 0;
long currentMillis = 0;

// One-second interval for measurements
int interval = 1000;

hd44780_I2Cexp lcd;

void wheelSpeed()
{
	duration++;
}

void EncoderInit()
{
	Direction = true;//default -> Forward
	pinMode(encoder0pinB,INPUT);
	attachInterrupt(digitalPinToInterrupt(7), wheelSpeed, CHANGE); //Pin 7 -> Interrupt 4
}

void advance()//Motor Forward
{
	digitalWrite(M_left,LOW);
	analogWrite(E_left,val_output);
}

void setup()
{
	lcd.begin(16, 2);
	lcd.noBacklight();
	pinMode(M_left, OUTPUT);   //L298P Control port settings DC motor driver board for the output mode
	pinMode(E_left, OUTPUT);
	Setpoint =1;  //Set the output value of the PID
	myPID.SetMode(AUTOMATIC);//PID is set to automatic mode
	myPID.SetSampleTime(100);//Set PID sampling frequency is 100ms
	EncoderInit();//Initialize the module
	
	previousMillis = millis();
}

void loop()
{
	advance();//Motor Forward
	currentMillis = millis();
	if (currentMillis - previousMillis > interval) {
		previousMillis = currentMillis;
	
	abs_duration=duration * 60 / 1920;
	
	result=myPID.Compute();//PID conversion is complete and returns 1
	if(result)
	{
		lcd.clear();
		lcd.print(abs_duration);	
		duration = 0; //Count clear, wait for the next count
	}
	}
}
*/




//-------------------------------------------------------------
//-------------- Interrupt Button -----------------------------
//-------------------------------------------------------------


#include <Arduino.h>
#include <Wire.h>

#define ledPin 4 //LED-Pin
#define buttonPin 7

boolean volatile status = false;

void ISRButton ()
{
	status = true;
	
};

void setup() {
	Serial.begin(9600);
	
	pinMode(ledPin, OUTPUT);
	pinMode(buttonPin, INPUT);

	attachInterrupt(digitalPinToInterrupt(buttonPin), ISRButton, RISING);
}
void loop() {
	if (status)
	{
		digitalWrite(ledPin, HIGH);
		delay(1000);
		status = false;
	}
	else 
	{
		digitalWrite(ledPin, LOW);
	}
	
	/*
	if (digitalRead(buttonPin) == HIGH)
	{
		digitalWrite(ledPin,HIGH);
	}
	else
	{
		digitalWrite(ledPin,LOW);	
	}
	
}

#include <Arduino.h>
#include <Wire.h>
#include <RTClib.h>
#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <SPI.h>

/* Global Variables*/
// Setting up pins for interrupts
const uint8_t date_btn = 2;
const uint8_t led_pin = 5;
//Interrupt flag
volatile uint8_t flag = 0;

//Delay for displaying the date
const uint16_t delay_time = 3000;

/* Setting up stuff for RTC*/

// Delcare rtc object
RTC_DS3231 rtc;

// 2D array for days of the week with a max of 12 characters
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

/* Setting up stuff for Dot Matrix Display*/

// Define hardware type, size, and output pins
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 4
#define CS_PIN 3

// Create new instace of MD_Parola class with hardware SPI connection
MD_Parola myDisplay = MD_Parola(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);

// Setup for software SPI (if needed)
// #define DATAPIN 2
// #define CLK_PIN 4
// MD_Parola myDisplay = MD_Parola(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);


void setup() {
  // Begin serial communication
  Serial.begin(115200);

  #ifdef ESP8266
    while(!Serial); // Wait for serial port to connect. Needed for native USB.
  #endif  

  // Check if RTC is connected
  if (!rtc.begin()) {
    Serial.println("RTC not found!");
    Serial.flush();
    abort();
  }

  // Check if RTC is running
  if (rtc.lostPower()){
    Serial.println("Power was lost. Setting time...");
    // Set the RTC to the date & time this sketch was compiled
    // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  }

  // Initialize the display
  myDisplay.begin();
  // Set brightness level 0-15
  myDisplay.setIntensity(0);
  //Clear the display
  myDisplay.displayClear();

  /* This whole section is dedicated to using interrupts*/

  // Clear bit 2 for data direction register in PORTD
  DDRD &= ~(1 << date_btn);
  // Set bit 2 for PORTD, effectively enabling itnernal pull-up resistor
  PORTD |= (1 << date_btn);
  
  //Set LED pin as output
  DDRD |= (1 << led_pin);

  // Falling edge trigger of INT0 generates interrupt by setting lowest bit of EICRA to 10
  EICRA |= (1 << ISC01); //set bit 1 of EICRA
  EICRA &= ~(1 << ISC00); // clear bit 0 of EICRA,  
  
  // Enable interrupts for INT0
  EIMSK |= (1 << INT0);

  //Enable global interrupts
  sei(); // to disable interrupts use cli();
}

void loop() {
  // Acquire the current date and time
  DateTime now = rtc.now();

  //buffer can be d#efined using following combinations:
  //hh - the hour with a leading zero (00 to 23)
  //mm - the minute with a leading zero (00 to 59)
  //ss - the whole second with a leading zero where applicable (00 to 59)
  //YYYY - the year as four digit number
  //YY - the year as two digit number (00-99)
  //MM - the month as number with a leading zero (01-12)
  //MMM - the abbreviated English month name ('Jan' to 'Dec')
  //DD - the day as number with a leading zero (01 to 31)
  //DDD - the abbreviated English day name ('Mon' to 'Sun')

  // Print current time
  char buf_time[] = "hh:mm";
  Serial.println(now.toString(buf_time)); 


  // Display the current time on the dot matrix display
  myDisplay.setTextAlignment(PA_CENTER);
  myDisplay.print(now.toString(buf_time));

  // check if flag is raised
  if (flag){
    // Print the current date
    PORTD ^= (1 << led_pin);
    Serial.println("Interrupt triggered");
    char buf_date[] = "DD MMM";
    Serial.println(now.toString(buf_date));

    // Display the current date on the dot matrix display
    myDisplay.displayClear();
    myDisplay.setTextAlignment(PA_LEFT);
    myDisplay.print(now.toString(buf_date));
    delay(delay_time);
    //Toggle the LED
    PORTD ^= (1 << led_pin);
    // Clear the flag
    flag = 0;
  }
  
}

// Interrupt service routine for INT0
ISR(INT0_vect){
  flag = 1;
  // PORTD ^= (1 << led_pin);
  // Serial.println("Interrupt triggered");
}
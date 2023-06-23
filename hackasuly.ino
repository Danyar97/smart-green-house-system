// librarys
#include <WiFi.h>
#include <WebServer.h>
#include "DHT.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#include "RTClib.h"
#include <ESP32Servo.h> // Recommended PWM GPIO pins on the ESP32 include 2,4,12-19,21-23,25-27,32-33

//////////////////////////////////////////////////////////////////////////////////////////////////////
//Define pins 

// temp & humidity
DHT DHT_IN(32, DHT22);
DHT DHT_OUT(33, DHT22);
float hum_IN, temp_IN, hum_OUT, temp_OUT;
OneWire oneWire(25);
DallasTemperature DS18B20(&oneWire);
float temp;

//////////////////////////////////////////////////////////////////////////////////////////////////////
//time RTC
RTC_DS3231 rtc;
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
int yyyy, MM, dd;
byte hh, mm, ss;

//////////////////////////////////////////////////////////////////////////////////////////////////////
//humidity inside soil
#define soilSensor_PIN 36
int soilSensor_STATE = 0;

//////////////////////////////////////////////////////////////////////////////////////////////////////
//relays
const byte SW_LENGTH = 5;
int SW_PIN[SW_LENGTH] = {14, 12, 13, 2, 4};
bool SW_STATUS[SW_LENGTH] = {LOW, LOW, LOW, HIGH, LOW};
bool SW_ON = LOW;

//////////////////////////////////////////////////////////////////////////////////////////////////////
//servos
Servo gate;
#define gate1_PIN 26
#define gate2_PIN 27
int openGate1 = 60;
int closeGate1 = 5;
int openGate2 = 80;
int closeGate2 = 5;
//////////////////////////////////////////////////////////////////////////////////////////////////////
//wifi connection
const char* ssid = "Gorannet";  // Enter SSID here
const char* password = "12345678";  //Enter Password here
WebServer server(80); // port

//////////////////////////////////////////////////////////////////////////////////////////////////////


void setup() {
  Serial.begin(115200); // baud rate serial monetor set
  WiFi.begin(ssid, password); 
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected..!"); 
  Serial.print("Got IP: ");  Serial.println(WiFi.localIP());

  server.on("/", handle_OnConnect);
  server.on("/SW1on", handle_SW1_On);
  server.on("/SW1off", handle_SW1_Off);
  server.on("/SW2on", handle_SW2_On);
  server.on("/SW2off", handle_SW2_Off);
  server.on("/SW3on", handle_SW3_On);
  server.on("/SW3off", handle_SW3_Off);
  server.on("/SW4on", handle_SW4_On);
  server.on("/SW4off", handle_SW4_Off);
  server.on("/SW5on", handle_SW5_On);
  server.on("/SW5off", handle_SW5_Off);
  server.onNotFound(handle_NotFound);
  server.begin();
  Serial.println("HTTP server started");

// run functions
  DHT_IN.begin(); 
  DHT_OUT.begin();
  DS18B20.begin();

  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
  }
  if (rtc.lostPower()) {
    Serial.println("RTC lost power, let's set the time!");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  for(byte PIN_ = 1; PIN_ <= SW_LENGTH; PIN_++){
    pinMode(SW_PIN[PIN_ - 1], OUTPUT);
    SW(PIN_, !SW_ON);
  }

  // Allow allocation of all timers
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);
  setGate(gate1_PIN, closeGate1);
  setGate(gate2_PIN, closeGate2);
}


void loop() {
  
}

void detDHTData() { //function reading dht data
  hum_IN = DHT_IN.readHumidity();
  temp_IN = DHT_IN.readTemperature();

  if (isnan(hum_IN) || isnan(temp_IN)) {
    Serial.println("Failed to read from DHT sensor IN!");
    return;
  }
  Serial.print("Humidity IN: ");
  Serial.print(hum_IN);
  Serial.print(" %\t");
  Serial.print("Temperature IN: ");
  Serial.print(temp_IN);
  Serial.println(" *C ");

  hum_OUT = DHT_OUT.readHumidity();
  temp_OUT = DHT_OUT.readTemperature();
  if (isnan(hum_OUT) || isnan(temp_OUT)) {
    Serial.println("Failed to read from DHT sensor OUT!");
    return;
  }
  Serial.print("Humidity OUT: ");
  Serial.print(hum_OUT);
  Serial.print(" %\t");
  Serial.print("Temperature OUT: ");
  Serial.print(temp_OUT);
  Serial.println(" *C ");
}

void getTempData() { //function reading temp data
  DS18B20.requestTemperatures();
  temp = DS18B20.getTempCByIndex(0);
  Serial.print(temp);
  Serial.println(" C");
}

void getRTC() { //function reading date&time from RTC 
  DateTime now = rtc.now();

  yyyy = now.year();
  MM = now.month();
  dd = now.day();
  Serial.print("Date ");
  Serial.print(yyyy);
  Serial.print("/");
  Serial.print(MM);
  Serial.print("/");
  Serial.print(dd);

  hh = now.hour();
  mm = now.minute();
  ss = now.second();
  Serial.print("   Time ");
  Serial.print(hh);
  Serial.print(":");
  Serial.print(mm);
  Serial.print(":");
  Serial.println(ss);

}

void getSoilData() { // function reading humidty from Soil 
  soilSensor_STATE = analogRead(soilSensor_PIN);
  //Serial.print("readpinnn  ");
  //Serial.println(soilSensor_STATE);
  soilSensor_STATE = map(soilSensor_STATE, 0, 4096, 100, 0);
  Serial.println("soil Sensor: " + String(soilSensor_STATE));
}

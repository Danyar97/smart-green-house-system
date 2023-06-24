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

String seeds="Seed: ";
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
//const char* ssid = "iQ-Bit Festival";  // Enter SSID here
// char* password = "iQ20232023";  //Enter Password here

WebServer server(80); // port

//////////////////////////////////////////////////////////////////////////////////////////////////////


void setup() {
  Serial.begin(115200);
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
//  server.on("/seed", handle_seed);

  server.onNotFound(handle_NotFound);
  server.begin();
  Serial.println("HTTP server started");

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
  detDHTData();
  getTempData();
  getSoilData();
  getRTC();
  server.handleClient();
}

void detDHTData() {
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

void getTempData() {
  DS18B20.requestTemperatures();
  temp = DS18B20.getTempCByIndex(0);
  Serial.print(temp);
  Serial.println(" C");
}

void getRTC() {
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

void getSoilData() {
  soilSensor_STATE = analogRead(soilSensor_PIN);
  //Serial.print("readpinnn  ");
  //Serial.println(soilSensor_STATE);
  soilSensor_STATE = map(soilSensor_STATE, 0, 4096, 100, 0);
  Serial.println("soil Sensor: " + String(soilSensor_STATE));
}

void SW(byte PIN_, bool onOff_){
  PIN_ -= 1;
  digitalWrite(SW_PIN[PIN_], onOff_);
  Serial.print("SW" + String(PIN_) + ": ");
  if(onOff_ == SW_ON){
    Serial.println("On");
    SW_STATUS[PIN_] = HIGH;
  }
  else{
    Serial.println("Off");
    SW_STATUS[PIN_] = LOW;
  }
}



void setGate(int PIN_, int POS_){
  gate.setPeriodHertz(50); // standard 50 hz servo
  gate.attach(PIN_, 500, 2400);
  // using default min/max of 1000us and 2000us
  // different servos may require different min/max settings
  // for an accurate 0 to 180 sweep
  delay(100);
  gate.write(POS_);
  delay(1000);
  gate.detach();
}

void handle_OnConnect() {
  server.send(200, "text/html", SendHTML());
}

// Fan 1 and Gate 1
void handle_SW1_On() {
  SW_STATUS[0] = HIGH;
  Serial.println("SW1: ON");
  SW(1, SW_ON);
  setGate(gate1_PIN, openGate1);
  server.send(200, "text/html", SendHTML());
}


void handle_SW1_Off() {
  SW_STATUS[0] = LOW;
  Serial.println("SW1: OFF");
  SW(1, !SW_ON);
  setGate(gate1_PIN, closeGate1);
  server.send(200, "text/html", SendHTML());
}



// Fan 2 and Gate 2
void handle_SW2_On() {
  SW_STATUS[1] = HIGH;
  Serial.println("SW2: ON");
  SW(2, SW_ON);
  setGate(gate2_PIN, openGate2);
  server.send(200, "text/html", SendHTML());
}
void handle_SW2_Off() {
  SW_STATUS[1] = LOW;
  Serial.println("SW2: OFF");
  SW(2, !SW_ON);
  setGate(gate2_PIN, closeGate2);
  server.send(200, "text/html", SendHTML());
}

// LED
void handle_SW3_On() {
  SW_STATUS[2] = HIGH;
  Serial.println("SW3: ON");
  SW(3, SW_ON);
  server.send(200, "text/html", SendHTML());
}
void handle_SW3_Off() {
  SW_STATUS[2] = LOW;
  Serial.println("SW3: OFF");
  SW(3, !SW_ON);
  server.send(200, "text/html", SendHTML());
}

// Water Pump
void handle_SW4_On() {
  SW_STATUS[3] = HIGH;
  Serial.println("SW4: OFF");
  SW(4, SW_ON);
  server.send(200, "text/html", SendHTML());
}
void handle_SW4_Off() {
  SW_STATUS[3] = LOW;
  Serial.println("SW4: ON");
  SW(4, !SW_ON);
  server.send(200, "text/html", SendHTML());
}

void handle_SW5_On() {
  SW_STATUS[4] = HIGH;
  Serial.println("SW5: ON");
  SW(5, SW_ON);
  server.send(200, "text/html", SendHTML());
}
void handle_SW5_Off() {
  SW_STATUS[4] = LOW;
  Serial.println("SW5: OFF");
  SW(5, !SW_ON);
  server.send(200, "text/html", SendHTML());
}

void handle_NotFound() {
  server.send(404, "text/plain", "Not found");
}
int cols=2;
String SendHTML() {
  String ptr = "<!DOCTYPE html> <html>\n";
  ptr += "<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";
  //ptr +="<head><meta http-equiv=\"refresh\" content=\"2; url=http://192.168.1.35 \" ></head>\n";
 ptr +="<head><meta http-equiv=\"refresh\" content=\"5\" ></head>\n";
  ptr += "<title>Smart</title>\n";
  ptr += "<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}\n";
  ptr += "body{margin-top: 50px;background-color: bisque;} h1 {color: #444444;margin: 50px auto 30px;} h3 {color: #444444;margin-bottom: 50px;}\n";
  ptr += ".button {display: block;width: 80px; background-color: bisque: none;color: white;padding: 13px 30px;text-decoration: none;font-size: 25px;margin: 0px auto 35px;cursor: pointer;border-radius: 4px;}\n";
  ptr += ".button-on {background-color: #7ecc52;}\n";
  ptr += ".button-on:active {background-color: #2980b9;}\n";
  ptr += ".button-off {background-color: #34495e;}\n";
  ptr += ".button-off:active {background-color: #2c3e50;}\n";
  ptr += "p {font-size: 14px;color: #888;margin-bottom: 10px;}\n";
  ptr += " table {";
  ptr += "    border-collapse: collapse;";
  ptr += "     margin: 0 auto;";
  ptr += " float:left;";
  ptr += "   width:32%;";
  ptr += "  }";

  ptr += "  th,";
  ptr += "  td {";
  ptr += "    border: 1px solid black;";
  ptr += "    padding: 10px;";
  ptr += "  }";
  
  ptr += "  th {";
  ptr += "    background-color: #7ecc52;"; //table color
  ptr += "    color: white;";
  ptr += "  }";

  ptr += "  tr:nth-child(even) {";
  ptr += "    background-color: #f2f2f2;";
  ptr += "  }";

 ptr += " .center {";
   ptr += " margin: 10px;";
ptr += " display: flex;";
ptr += " flex-direction: column;";
 ptr += "}";
 ptr += ".container {";
  ptr += "          display: flex;";
  ptr += "          flex-direction: column;";
  ptr += "          row-gap: 20px;";
  ptr += "      }";
 ptr += " .center>table {";
  ptr += "          margin-top: 10px;";
 ptr += "       }";
 ptr += "       .text-container>h1 {";
 ptr += "           font-size: 40px;";
 ptr += "       }";
 ptr += "        .title-word {";
 ptr += "            animation: color-animation 4s linear infinite;";
 ptr += "}";
 ptr += ".title-word-1 {";
 ptr += "            --color-1: #DF8453;";
  ptr += "           --color-2: #3D8DAE;";
 ptr += "    --color-3: #E4A9A8;";
  ptr += "       }";

  ptr += "       .title-word-2 {";
    ptr += "         --color-1: #DBAD4A;";
   ptr += "          --color-2: #ACCFCB;";
   ptr += "          --color-3: #17494D;";
   ptr += "      }";

    ptr += "     .title-word-3 {";
     ptr += "        --color-1: #089637;";
    ptr += "         --color-2: #7d9b71;";
    ptr += "         --color-3: #739c81;";
    ptr += "     }";

      ptr += "   .title-word-4 {";
     ptr += "        --color-1: #3D8DAE;";
     ptr += "        --color-2: #DF8453;";
    ptr += "         --color-3: #E4A9A8;";
    ptr += "     }";

     ptr += "    @keyframes color-animation {";
     ptr += "        0% {";
      ptr += "           color: var(--color-1)";
      ptr += "       }";

      ptr += "       32% {";
       ptr += "          color: var(--color-1)";
       ptr += "      }";

       ptr += "      33% {";
        ptr += "         color: var(--color-2)";
        ptr += "     }";

        ptr += "     65% {";
         ptr += "        color: var(--color-2)";
         ptr += "    }";

          ptr += "   66% {";
          ptr += "       color: var(--color-3)";
          ptr += "   }";

         ptr += "    99% {";
          ptr += "       color: var(--color-3)";
          ptr += "   }";

         ptr += "    100% {";
        ptr += "         color: var(--color-1)";
       ptr += "      }";
       ptr += "  }";

        

       ptr += "  .containert {";
       ptr += "      display: grid;";
       ptr += "      place-items: center;";
       ptr += "      text-align: center;";
        ptr += "     height: 100px";
       ptr += "  }";

      ptr += "   .titlet {";
      ptr += "       font-family: \"Montserrat\", sans-serif;";
      ptr += "       font-weight: 80;";
       ptr += "      font-size: 60px;";
       ptr += "      text-transform: uppercase;";
       ptr += "  }";





  ptr += "</style>\n";
  ptr += "</head>\n";
  ptr += "<body>\n";
 

 ptr += " <section class=\"text-container\">";
  ptr += "      <div class=\"containert\">";
   ptr += "         <label class=\"titlet\">";
   ptr += "             <span class=\"title-word title-word-1\">Smart</span>";
     ptr += "           <span class=\"title-word title-word-2\">Green</span>";
     ptr += "           <span class=\"title-word title-word-3\">House</span>";
      ptr += "          <span class=\"title-word title-word-4\">System</span>";
      ptr += "      </label>";
     ptr += "   </div>";
     ptr += "   <br>";
 ptr += "<h2>Date " + String(yyyy) + "/" + String(MM) + "/" + String(dd) +"</h2>";
 ptr += "<h2>Time " + String(hh) + ":" + String(mm) + ":" + String(ss) +"</h2>";

 ptr +=  " </section>";

 
//ptr += "<h2>Date 27 / 4 / 2023 </h2>";

ptr+= "<div class=container>";

ptr+= "<table>";
ptr+= "<tr>";
ptr+= "            <th colspan="+ String(cols) +">Inside</th>";
ptr+= "        </tr>";
ptr+=  " <tr>";
ptr+=           " <th>Humidity</th> <th>Temprature</th>";
ptr+=       " </tr>";
ptr+=       " <tr>";
ptr+=           " <td>"+ String(hum_IN) +"%</td> <td>" + String(temp_IN) + "C</td>";
ptr+=       " </tr>";
ptr+=       " </table>";


ptr+= "<table>";
ptr+= "<tr>";
ptr+= "            <th colspan="+ String(cols) +">Outside</th>";
ptr+= "        </tr>";
ptr+=  " <tr>";
ptr+=           " <th>Humidity</th> <th>Temprature</th>";
ptr+=       " </tr>";
ptr+=       " <tr>";
ptr+=           " <td>"+ String(hum_OUT) +"%</td> <td>" + String(temp_OUT) + "C</td>";
ptr+=       " </tr>";
ptr+=       " </table>";


ptr+= "<table>";
ptr+= "<tr>";
ptr+= "            <th colspan="+ String(cols) +">Soil</th>";
ptr+= "        </tr>";
ptr+=  " <tr>";
ptr+=           " <th>Humidity</th> <th>Temprature</th>";
ptr+=       " </tr>";
ptr+=       " <tr>";
ptr+=           " <td>"+ String(soilSensor_STATE) +"%</td> <td>" + String(temp) + "C</td>";
ptr+=       " </tr>";
ptr+=       " </table>";

ptr+="</div>";



//  ptr += "<p>Humidity IN: " + String(hum_IN) + "%</p>";
 // ptr += "<p>Temprature IN: " + String(temp_IN) + "C</p>";
//  ptr += "<p>Humidity OUT: " + String(hum_OUT) + "%</p>";
//ptr += "<p>Temprature OUT: " + String(temp_OUT) + "C</p>";
 // ptr += "<p>Temprature: " + String(temp) + "C</p>";
//  ptr += "<p>Soil Sensor: " + String(soilSensor_STATE) + "%</p>";

ptr+= "<div class=center >";
ptr+= "<table>";
  ptr+= "          <tr>";
  ptr+= "              <th colspan=4> Seeds </th>";
   ptr+= "         </tr>";
   ptr+= "         <tr>";
   ptr+= "             <th> <select id=\"myAns\">";
     ptr+= "                     <option value=\"choose\"> --choose Seed -- </option>";
      ptr+= "                    <option value=\"Strawberry\"> Strawberry </option>";
       ptr+= "                   <option value=\"Tomato\"> Tomato </option>";
        ptr+= "                   <option value=\"Cucumber\"> Cucumber </option>";
        ptr+= "                   <option value=\"Flower\"> Flower </option>";
        ptr+= "              </select>";
          ptr+= "            </th>";
 
     ptr+= "       </tr>";


     ptr+= "  </table>";



 ptr+= "<table>";
 ptr+= "<tr>";
ptr+= "            <th colspan="+ String(4) +">Relays Control</th>";
ptr+= "        </tr>";
ptr+=  " <tr>";
ptr+=           " <th>Air IN</th> <th>Air Out</th> <th>Light</th> <th>Water Pump</th>";
ptr+=       " </tr>";
ptr+=       " <tr>";

  if (SW_STATUS[0]) {
    ptr += "<td><a class=\"button button-off\" href=\"/SW1off\">OFF</a></td>";
  }
  else {
    ptr += "<td><a class=\"button button-on\" href=\"/SW1on\">ON</a></td></td>";
  }

  if (SW_STATUS[1]) {
    ptr += "<td><a class=\"button button-off\" href=\"/SW2off\">OFF</a></td>";
  }
  else {
    ptr += "<td><a class=\"button button-on\" href=\"/SW2on\">ON</a></td>";
  }

 // if (SW_STATUS[2]) {
 //   ptr += "<p>SW3: ON</p><a class=\"button button-off\" href=\"/SW3off\">OFF</a>\n";
 // }
//else {
//    ptr += "<p>SW3: OFF</p><a class=\"button button-on\" href=\"/SW3on\">ON</a>\n";
//  }

  if (SW_STATUS[3]) {
    ptr += "<td><a class=\"button button-off\" href=\"/SW4off\">OFF</a></td>";
  }
  else {
    ptr += "<td><a class=\"button button-on\" href=\"/SW4on\">ON</a></td>";
  }

  if (SW_STATUS[4]) {
    ptr += "<td><a class=\"button button-off\" href=\"/SW5off\">OFF</a></td>";
  }
  else {
    ptr += "<td><a class=\"button button-on\" href=\"/SW5on\">ON</a></td>";
  }





ptr+=       " </tr>";
ptr+=       " </table>";
ptr+="</div>";



 // ptr += "<a class=\"button button-on\" href=\"/alloff\">Close All</a>\n";

  ptr += "</body>\n";
  ptr += "</html>\n";
  return ptr;
}

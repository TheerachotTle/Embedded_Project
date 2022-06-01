//
// Copyright 2015 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

// FirebaseDemo_ESP8266 is a sample that demo the different functions
// of the FirebaseArduino API.

#include <ESP8266WiFi.h>
#include <FirebaseArduino.h>
#include <SoftwareSerial.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
SoftwareSerial NodeSerial(D2, D3); // RX | TX

// Set these to run example.
#define FIREBASE_HOST "imbedded-system-default-rtdb.asia-southeast1.firebasedatabase.app"
#define FIREBASE_AUTH "5AINbOtk6VstLh2KrVqhWRDHSnYgcrxbSfnN1S2C"
#define WIFI_SSID "TitleOfIphone"
#define WIFI_PASSWORD "Who-are-you"

int led = D5;

const char *ssid     = "TitleOfIphone";
const char *password = "Who-are-you";
const long utcOffsetInSeconds = 25200;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

void setup() {
  Serial.begin(115200);
  NodeSerial.begin(115200);
  pinMode(led,OUTPUT);
  // connect to wifi.
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("connecting");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println();
  Serial.print("connected: ");
  Serial.println(WiFi.localIP());
  
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  timeClient.begin();
}

int human,prev_d,d,r=0;
float light_per;
unsigned long delayTime,lastTime;

const byte maxInts = 32;
char receivedInformation[maxInts];
char tempInformation[maxInts];

bool newData = false;
bool notInRange = false;
bool InRange;
unsigned long clockTime;

bool checkTime(){
  if(!Firebase.getBool("OpeningTime")){
    return true;
  }
  if(Firebase.getBool("update")){
    clockTime = millis();
    Firebase.setBool("update",false);
    if(inTimeRange()){
      notInRange = false;
      return true;
    }else{
      notInRange = true;
      return false;
    }
  }
  else if(notInRange){
    return false;
  }else{
    if(millis()-clockTime>=5000){
      notInRange = !inTimeRange();
      clockTime = millis();
      return !notInRange;
    }
    return true;
  }
}
bool inTimeRange(){
  timeClient.update();
  int start = Firebase.getInt("TimeRange/StartHour")*60 +Firebase.getInt("TimeRange/StartMin");
  int ending = Firebase.getInt("TimeRange/EndHour")*60 +Firebase.getInt("TimeRange/EndMin");
  int now = timeClient.getHours()*60 + timeClient.getMinutes();
  if(start>ending){
    ending +=1440;
    now +=1440;
  }
  return now>=start&&now<ending;  
}
void CountHuman(){
  if(Firebase.getBool("System") && InRange && Firebase.getBool("MotionSensor") && prev_d!=d){
    if(d!=0 && d<50 && 1.0*r/4100 <Firebase.getFloat("Light")){
      human++;
      delayTime = 5000;
      lastTime = millis();
    }
  }
  prev_d=d;
}
void BlinkLed(){
  if(Firebase.getBool("System") && InRange){
    if(Firebase.getBool("MotionSensor") && d!=0 && d<50 && prev_d!=d && (1.0*r/4100)*100 <Firebase.getFloat("Light")){
      digitalWrite(led,HIGH);
      human++;
      delayTime = 5000;
      lastTime = millis();
      while(Firebase.getBool("System") && Firebase.getBool("MotionSensor") && millis()-lastTime<delayTime && (1.0*r/4100)*100 <Firebase.getFloat("Light")){
        loop1();
        CountHuman();
      }
      digitalWrite(led,LOW);
    }
    else if(!Firebase.getBool("MotionSensor") && (1.0*r/4100)*100 <Firebase.getFloat("Light")){
      while((1.0*r/4100)*100 <Firebase.getFloat("Light") && Firebase.getBool("System") && checkTime()&& !Firebase.getBool("MotionSensor")){
        digitalWrite(led,HIGH);
        loop1();
      }
      digitalWrite(led,LOW);
  }
}
}
void receiveInfo(){
  static bool receivingInProgress = false;
  static byte index = 0;
  char startMarker = '<';
  char endMarker = '>';
  char rc;

  while (NodeSerial.available() > 0 && newData == false){
    rc = NodeSerial.read();

    //Find Start
    if (rc == startMarker){
      receivingInProgress = true;
    }
    //Read Information
    else if (rc != endMarker){
      receivedInformation[index] = rc;
      index++;
      if (index >= maxInts){
        index = maxInts - 1;
      }
    }
    //Find End
    else if (rc == endMarker){
      receivedInformation[index] = '\0';
      receivingInProgress = false;
      index = 0;
      newData = true;
    }
  }
}

void parseData() {
  
  char * strtokIndex;

  strtokIndex = strtok(tempInformation,",");
  r = atoi(strtokIndex);
  strtokIndex = strtok(NULL,",");
  d = atoi(strtokIndex);
  
}

void showParsedData() {
  
  Serial.print("Light ");
  Serial.println(r);
  Serial.print("Distance ");
  Serial.println(d);
  
}
void loop1(){
  while(newData == false){
    receiveInfo();
  }
  if (newData == true){
    strcpy(tempInformation,receivedInformation);
    parseData();
    showParsedData();
    newData = false;
  }
}
void loop() {
    InRange = checkTime();
    loop1();
    BlinkLed();
    Firebase.setInt("Human",Firebase.getInt("Human")+human);
    human=0;
}

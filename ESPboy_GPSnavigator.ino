/*
ESPboy GPS navigator by RomanS

ESPboy project page:
https://hackaday.io/project/164830-espboy-beyond-the-games-platform-with-wifi
*/


#include <TinyGPS++.h>
#include <SoftwareSerial.h>
#include <ESP_EEPROM.h>

#include "lib/ESPboyInit.h"
#include "lib/ESPboyInit.cpp"

ADC_MODE(ADC_VCC);

#define GPSdatatimeout  2000
#define TXpin             D8
#define RXpin             D6
#define GPSBaud           9600

double waypoint[5][2];
uint8_t currentwaypoint = 0;
uint8_t displaymode = 0;

TinyGPSPlus gps;
SoftwareSerial ss(RXpin, TXpin);

ESPboyInit myESPboy;

struct gpsstruct{
  double lat; //Latitude in degrees (double)
  double lng; //Longitude in degrees (double)
  double alt; //Altitude in meters (double)
  uint16_t year; // Year (2000+) (u16)
  uint8_t month; // Month (1-12) (u8)
  uint8_t day; // Day (1-31) (u8)
  uint8_t hour; // Hour (0-23) (u8)
  uint8_t min; // Minute (0-59) (u8)
  uint8_t sec; // Second (0-59) (u8)
  double speedknots; // Speed in knots (double)
  double speedkmph; // Speed in kilometers per hour (double)
  double course; // Course in degrees (double)
  uint32_t sat; // Number of satellites in use (u32)
  uint32_t hdop; // Horizontal Dim. of Precision (100ths-i32)

  boolean coordisvalid;
  boolean datetimeisvalid;
  boolean speedisvalid;
  boolean courseisvalid;
  boolean altisvalid;
  boolean satisvalid;
  boolean hdopisvalid;

  boolean coordisupdate;
  boolean datetimeisupdate;
  boolean speedisupdate;
  boolean courseisupdate;
  boolean altisupdate;
  boolean satisupdate;
  boolean hdopisupdate;

  double coordage;
  double datetimeage;
  double speedage;
  double courseage;
  double altage;
  double satage;
  double hdopage;
} gpsdata;




void drawled(){
  if (gpsdata.coordage > GPSdatatimeout || !gpsdata.coordisvalid){
     myESPboy.playTone (600, 20);
     myESPboy.myLED.setRGB(10,0,0);
  }
  else{
     myESPboy.myLED.setRGB(0,10,0);
  }
}



void drawtft(){
 myESPboy.tft.fillRect(0, 0, 128, 10, TFT_BLACK);
 myESPboy.tft.fillRect(0, 120, 128, 8, TFT_BLACK);
 myESPboy.tft.setTextSize(1);
  myESPboy.tft.setTextColor(TFT_WHITE);
  myESPboy.tft.setCursor(0, 0);
  if (gpsdata.datetimeisvalid){
    if (gpsdata.day<10) myESPboy.tft.print ("0");
    myESPboy.tft.print (gpsdata.day);
    myESPboy.tft.print (".");
    if (gpsdata.month<10) myESPboy.tft.print ("0");
    myESPboy.tft.print (gpsdata.month);
    myESPboy.tft.print (".");
    myESPboy.tft.print (gpsdata.year);
    myESPboy.tft.print ("  UTC ");
    if (gpsdata.hour<10) myESPboy.tft.print ("0");
    myESPboy.tft.print (gpsdata.hour);
    myESPboy.tft.print (":");
    if (gpsdata.min<10) myESPboy.tft.print ("0");
    myESPboy.tft.print (gpsdata.min);}
  else myESPboy.tft.print ("--.--.----  UTC --:--");
  
  myESPboy.tft.setCursor(0, 120);
  myESPboy.tft.print ("D:");
  myESPboy.tft.print (gps.charsProcessed());
  myESPboy.tft.print ("  E:");
  myESPboy.tft.print (gps.failedChecksum());
  myESPboy.tft.print("  B:");
  myESPboy.tft.print(map(ESP.getVcc (), 2700, 4200, 0, 100));
  myESPboy.tft.print("%");

   
 if (displaymode == 0){
  myESPboy.tft.fillRect(0, 10, 128, 10, TFT_BLACK);
  myESPboy.tft.fillRect(46, 20, 82, 100, TFT_BLACK);
  
  myESPboy.tft.setTextSize(1);
  myESPboy.tft.setTextColor(TFT_MAGENTA);
  myESPboy.tft.setCursor(0, 13);
  myESPboy.tft.print("Sat:");
  if(gpsdata.satisvalid) myESPboy.tft.print (gpsdata.sat);
  else myESPboy.tft.print ("--");
  myESPboy.tft.print("  Prc:");
  if (gpsdata.hdopisvalid) myESPboy.tft.print (gpsdata.hdop);
  else myESPboy.tft.print ("--");
  myESPboy.tft.print("  WP:");
  myESPboy.tft.print(currentwaypoint+1);
  
  myESPboy.tft.setTextSize(2);
  myESPboy.tft.setTextColor(TFT_YELLOW);
  myESPboy.tft.setCursor(0, 27);
  myESPboy.tft.print("Lat ");
  if(gpsdata.coordisvalid) myESPboy.tft.print (gpsdata.lat);
  else myESPboy.tft.print ("--");
  
  myESPboy.tft.setCursor(0, 45);
  myESPboy.tft.print("Lng ");
  if(gpsdata.coordisvalid) myESPboy.tft.print (gpsdata.lng);
  else myESPboy.tft.print ("--");
  
  myESPboy.tft.setTextColor(TFT_GREEN);  
  myESPboy.tft.setCursor(0, 63);
  myESPboy.tft.print("Spd ");
  if (gpsdata.speedisvalid) myESPboy.tft.print (round (gpsdata.speedkmph));
  else myESPboy.tft.print ("--");
  
  myESPboy.tft.setCursor(0, 81);
  myESPboy.tft.print("Alt ");
  if (gpsdata.hdopisvalid) myESPboy.tft.print (round (gpsdata.alt));
  else myESPboy.tft.print ("--");
  
  myESPboy.tft.setTextSize(1);
  myESPboy.tft.setTextColor(TFT_RED);
  myESPboy.tft.setCursor(0, 104);
  myESPboy.tft.fillRect(0, 104, 128, 10, TFT_BLACK);
  if (gpsdata.coordage > GPSdatatimeout) {
      myESPboy.tft.print(" GPS connection lost!");
  }
  
  else{
     myESPboy.tft.print(" ");
     for (int i = 0; i<5; i++){
        if (i == currentwaypoint) myESPboy.tft.setTextColor(TFT_YELLOW); 
        else myESPboy.tft.setTextColor(TFT_RED);
        myESPboy.tft.print("[");
        myESPboy.tft.print(i+1);
        myESPboy.tft.print("] ");
     }   
  }
 }

 
 else{
  myESPboy.tft.fillRect(0, 10, 128, 108, TFT_BLACK);
  myESPboy.tft.setTextSize(2);
  myESPboy.tft.setTextColor(TFT_YELLOW);
  myESPboy.tft.setCursor(0, 12);
  myESPboy.tft.print("Spd ");
  if (gpsdata.speedisvalid) myESPboy.tft.print ((uint8_t)round(gpsdata.speedkmph));
  else myESPboy.tft.print ("--");
  
  myESPboy.tft.setTextSize(1);
  myESPboy.tft.setCursor(0, 30);
  myESPboy.tft.print("S:");
  if(gpsdata.satisvalid) myESPboy.tft.print (gpsdata.sat);
  else myESPboy.tft.print ("--");
  myESPboy.tft.print(" Lt:");
  if(gpsdata.coordisvalid) myESPboy.tft.print (gpsdata.lat);
  else myESPboy.tft.print ("--");
  myESPboy.tft.print(" Lg:");
  if(gpsdata.coordisvalid) myESPboy.tft.print (gpsdata.lng);
  else myESPboy.tft.print ("--");
  
  myESPboy.tft.setTextSize(2);
  myESPboy.tft.setTextColor(TFT_MAGENTA);
  myESPboy.tft.setCursor(0, 42);
  myESPboy.tft.print("Point ");
  myESPboy.tft.print(currentwaypoint+1);

  myESPboy.tft.setCursor(0, 58);
  myESPboy.tft.setTextSize(1);
  myESPboy.tft.print("Lt:");
  myESPboy.tft.print (waypoint[currentwaypoint][0]);
  myESPboy.tft.print(" Lg:");
  myESPboy.tft.print (waypoint[currentwaypoint][1]);

  myESPboy.tft.setTextSize(2);
  myESPboy.tft.setTextColor(TFT_GREEN);
  myESPboy.tft.setCursor(0, 69);
  myESPboy.tft.print ("Dst");
  double dist = round (gps.distanceBetween (gps.location.lat(), gps.location.lng(), waypoint[currentwaypoint][0], waypoint[currentwaypoint][1]));
  if (dist < 999999){ myESPboy.tft.print (" "); myESPboy.tft.print ((uint16_t)dist);}
  else myESPboy.tft.print (">999999");
  myESPboy.tft.setCursor(0, 85);
  myESPboy.tft.print ("Crs ");
  double cours = round (gps.courseTo (gps.location.lat(), gps.location.lng(), waypoint[currentwaypoint][0], waypoint[currentwaypoint][1])); 
  myESPboy.tft.print ((uint16_t)cours);
  myESPboy.tft.setCursor(0, 101);
  myESPboy.tft.print ("Dir ");
  myESPboy.tft.print(gps.cardinal(cours));
  
  myESPboy.tft.setTextSize(1);
  myESPboy.tft.setTextColor(TFT_RED);
  myESPboy.tft.setCursor(0, 120);
  if (gpsdata.coordage > GPSdatatimeout) {
    myESPboy.tft.fillRect(0, 120, 128, 8, TFT_BLACK);
    myESPboy.tft.print(" GPS connection lost!");}
 }
}


void esp_eeprom_save(){
  EEPROM.put (10, waypoint);
  EEPROM.commit();
}


void esp_eeprom_load(){
    EEPROM.get (10, waypoint);
}


void esp_eeprom_check(){
  char ch1, ch2;
  EEPROM.get (1,ch1);
  EEPROM.get (2,ch2);
  
  if(ch1 != 'G' || ch2 != 'S'){
    Serial.println("SETTING ZERO");
    EEPROM.put (1, 'G');
    EEPROM.put (2, 'S');
    memset (waypoint, sizeof(waypoint), 0);
    esp_eeprom_save();
  }
  else {
    Serial.println("LOADING DATA");
    esp_eeprom_load();
  }
}


void fillgpsstruct(){
  gpsdata.lat = gps.location.lat(); // Latitude in degrees (double)
  gpsdata.lng = gps.location.lng(); // Longitude in degrees (double)
  gpsdata.alt = gps.altitude.meters(); // Altitude in meters (double)
  gpsdata.year = gps.date.year(); // Year (2000+) (u16)
  gpsdata.month = gps.date.month(); // Month (1-12) (u8)
  gpsdata.day = gps.date.day(); // Day (1-31) (u8)
  gpsdata.hour = gps.time.hour(); // Hour (0-23) (u8)
  gpsdata.min = gps.time.minute(); // Minute (0-59) (u8)
  gpsdata.sec = gps.time.second(); // Second (0-59) (u8)
  gpsdata.speedknots = gps.speed.knots(); // Speed in knots (double)
  gpsdata.speedkmph = gps.speed.kmph(); // Speed in kilometers per hour (double)
  gpsdata.course = gps.course.deg(); // Course in degrees (double)
  gpsdata.sat = gps.satellites.value(); // Number of satellites in use (u32)
  gpsdata.hdop = gps.hdop.value(); // Horizontal Dim. of Precision (100ths-i32)

  gpsdata.coordisvalid = gps.location.isValid();
  gpsdata.speedisvalid = gps.speed.isValid();
  gpsdata.courseisvalid = gps.course.isValid();
  gpsdata.altisvalid = gps.altitude.isValid();
  gpsdata.satisvalid = gps.satellites.isValid();
  gpsdata.hdopisvalid = gps.hdop.isValid();
  gpsdata.datetimeisvalid = gps.time.isValid();

  gpsdata.coordisupdate = gps.location.isUpdated();
  gpsdata.speedisupdate = gps.speed.isUpdated();
  gpsdata.courseisupdate = gps.course.isUpdated();
  gpsdata.altisupdate = gps.altitude.isUpdated();
  gpsdata.satisupdate = gps.satellites.isUpdated();
  gpsdata.hdopisupdate = gps.hdop.isUpdated();
  gpsdata.datetimeisupdate = gps.time.isUpdated();

  gpsdata.coordage = gps.location.age();
  gpsdata.speedage = gps.speed.age();
  gpsdata.courseage = gps.course.age();
  gpsdata.altage = gps.altitude.age();
  gpsdata.satage = gps.satellites.age();
  gpsdata.hdopage = gps.hdop.age();
  gpsdata.datetimeage = gps.time.age();
}
  

void runButtonsCommand(uint8_t bt){
  myESPboy.playTone (800, 20);
  myESPboy.myLED.setRGB (0,0,20); 
if (displaymode == 0){ 
  if ((bt&PAD_UP || bt&PAD_RIGHT)  && currentwaypoint != 4) currentwaypoint++;
  if ((bt&PAD_DOWN || bt&PAD_LEFT) && currentwaypoint != 0) currentwaypoint--; 
  if (bt&PAD_ACT){
      waypoint[currentwaypoint][0] = gpsdata.lat;
      waypoint[currentwaypoint][1] = gpsdata.lng;
      myESPboy.tft.fillRect(0, 104, 128, 10, TFT_BLACK);
      myESPboy.tft.setTextSize(1);
      myESPboy.tft.setTextColor(TFT_MAGENTA);
      myESPboy.tft.setCursor(0, 104);
      myESPboy.tft.print ("  waypoint ");
      myESPboy.tft.print (currentwaypoint+1);
      myESPboy.tft.print (" stored");
      myESPboy.playTone (300, 100);
      esp_eeprom_save();
      smartDelay (500);
   }
   if (bt&PAD_ESC) {
     displaymode++;
     if (displaymode >1)  displaymode = 0;
     myESPboy.tft.fillScreen(TFT_BLACK);}
}

else{
   if (bt&PAD_ESC) {
     displaymode++;
     if (displaymode >1)  
     displaymode = 0; 
     myESPboy.tft.fillScreen(TFT_BLACK);
     }
   if (((bt&PAD_UP) || (bt&PAD_RIGHT))  && currentwaypoint != 4) currentwaypoint++;
   if (((bt&PAD_DOWN) || (bt&PAD_LEFT)) && currentwaypoint != 0) currentwaypoint--;    
}

  //while (!myESPboy.getKeys()) smartDelay(0);
  drawled();
  drawtft();
  delay(200); 
}


void smartDelay(unsigned long ms){
  static unsigned long start;
  start = millis();
  do {
    while (ss.available())
      gps.encode(ss.read());
      delay (4);
  } while (millis() - start < ms);
}



void setup(){
  Serial.begin(115200); //serial init
  ss.begin(GPSBaud);   //software serial init

  //Init ESPboy
  myESPboy.begin("GPS Navigator");
  
  pinMode (RXpin, INPUT_PULLUP);
 
//load last waypoint state from eeprom
  EEPROM.begin(10 + sizeof (waypoint));
  esp_eeprom_check();
}



void loop(){
 static long count;
   while (ss.available()) gps.encode(ss.read());
   fillgpsstruct(); 
   uint8_t bt=myESPboy.getKeys();
   if (bt) runButtonsCommand(bt);
   if (millis() > count + 3000){
     count = millis();
     drawtft(); 
     drawled();
    }
}

/*
ESPboy Navigator by RomanS

ESPboy project page:
https://hackaday.io/project/164830-espboy-beyond-the-games-platform-with-wifi
*/

#include <Adafruit_NeoPixel.h>
#include <Adafruit_MCP23017.h>
#include <TinyGPS++.h>
#include <SoftwareSerial.h>
#include <TFT_eSPI.h>
#include <ESP8266WiFi.h>
#include "ESPboyLogo.h"
#include <ESP_EEPROM.h>
#include "ESPboyOTA.h"

#define MCP23017address 0 //actually it's 0x20 but in <Adafruit_MCP23017.h> there is (x|0x20)
#define GPSdatatimeout  2000
#define LEDquantity     1

#define PAD_LEFT        0x01
#define PAD_UP          0x02
#define PAD_DOWN        0x04
#define PAD_RIGHT       0x08
#define PAD_ACT         0x10
#define PAD_ESC         0x20
#define PAD_LFT         0x40
#define PAD_RGT         0x80
#define PAD_ANY         0xff

//SPI for LCD
#define csTFTMCP23017pin  8 //chip select pin on the MCP23017 for TFT display

#define LEDpin            D4
#define SOUNDpin          D3
#define TXpin             A0
#define RXpin             D6
#define GPSBaud           9600

double waypoint[5][2];
uint8_t currentwaypoint = 0;
uint8_t displaymode = 0;
static uint8_t esp_eeprom_needsaving = 0;


Adafruit_NeoPixel pixels = Adafruit_NeoPixel(LEDquantity, LEDpin, NEO_GRB + NEO_KHZ800);
TinyGPSPlus gps;
SoftwareSerial ss(RXpin, TXpin);
Adafruit_MCP23017 mcp;
TFT_eSPI tft = TFT_eSPI();
ESPboyOTA* OTAobj = NULL;

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
     tone (SOUNDpin, 600, 20);
     pixels.setPixelColor(0, pixels.Color(10,0,0));
  }
  else{
     pixels.setPixelColor(0, pixels.Color(0,10,0));
  }
  pixels.show();
}



void drawtft(){
 tft.fillRect(0, 0, 128, 10, TFT_BLACK);
 tft.fillRect(0, 120, 128, 8, TFT_BLACK);
 tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(0, 0);
  if (gpsdata.datetimeisvalid){
    if (gpsdata.day<10) tft.print ("0");
    tft.print (gpsdata.day);
    tft.print (".");
    if (gpsdata.month<10) tft.print ("0");
    tft.print (gpsdata.month);
    tft.print (".");
    tft.print (gpsdata.year);
    tft.print ("  UTC ");
    if (gpsdata.hour<10) tft.print ("0");
    tft.print (gpsdata.hour);
    tft.print (":");
    if (gpsdata.min<10) tft.print ("0");
    tft.print (gpsdata.min);}
  else tft.print ("--.--.----  UTC --:--");
  
  tft.setCursor(0, 120);
  tft.print ("D:");
  tft.print (gps.charsProcessed());
  tft.print ("  E:");
  tft.print (gps.failedChecksum());
  tft.print("  B:");
  tft.print(map(analogRead(A0), 820, 1024, 0, 100));
  tft.print("%");

   
 if (displaymode == 0){
  tft.fillRect(0, 10, 128, 10, TFT_BLACK);
  tft.fillRect(46, 20, 82, 100, TFT_BLACK);
  
  tft.setTextSize(1);
  tft.setTextColor(TFT_MAGENTA);
  tft.setCursor(0, 13);
  tft.print("Sat:");
  if(gpsdata.satisvalid) tft.print (gpsdata.sat);
  else tft.print ("--");
  tft.print("  Prc:");
  if (gpsdata.hdopisvalid) tft.print (gpsdata.hdop);
  else tft.print ("--");
  tft.print("  WP:");
tft.print(currentwaypoint+1);
  
  tft.setTextSize(2);
  tft.setTextColor(TFT_YELLOW);
  tft.setCursor(0, 27);
  tft.print("Lat ");
  if(gpsdata.coordisvalid) tft.print (gpsdata.lat);
  else tft.print ("--");
  
  tft.setCursor(0, 45);
  tft.print("Lng ");
  if(gpsdata.coordisvalid) tft.print (gpsdata.lng);
  else tft.print ("--");
  
  tft.setTextColor(TFT_GREEN);  
  tft.setCursor(0, 63);
  tft.print("Spd ");
  if (gpsdata.speedisvalid) tft.print (round (gpsdata.speedkmph));
  else tft.print ("--");
  
  tft.setCursor(0, 81);
  tft.print("Alt ");
  if (gpsdata.hdopisvalid) tft.print (round (gpsdata.alt));
  else tft.print ("--");
  
  tft.setTextSize(1);
  tft.setTextColor(TFT_RED);
  tft.setCursor(0, 104);
  tft.fillRect(0, 104, 128, 10, TFT_BLACK);
  if (gpsdata.coordage > GPSdatatimeout) {
      tft.print(" GPS connection lost!");
  }
  
  else{
     tft.print(" ");
     for (int i = 0; i<5; i++){
        if (i == currentwaypoint) tft.setTextColor(TFT_YELLOW); 
        else tft.setTextColor(TFT_RED);
        tft.print("[");
        tft.print(i+1);
        tft.print("] ");
     }   
  }
 }

 
 else{
  tft.fillRect(0, 10, 128, 108, TFT_BLACK);
  tft.setTextSize(2);
  tft.setTextColor(TFT_YELLOW);
  tft.setCursor(0, 12);
  tft.print("Spd ");
  if (gpsdata.speedisvalid) tft.print ((uint8_t)round(gpsdata.speedkmph));
  else tft.print ("--");
  
  tft.setTextSize(1);
  tft.setCursor(0, 30);
  tft.print("S:");
  if(gpsdata.satisvalid) tft.print (gpsdata.sat);
  else tft.print ("--");
  tft.print(" Lt:");
  if(gpsdata.coordisvalid) tft.print (gpsdata.lat);
  else tft.print ("--");
  tft.print(" Lg:");
  if(gpsdata.coordisvalid) tft.print (gpsdata.lng);
  else tft.print ("--");
  
  tft.setTextSize(2);
  tft.setTextColor(TFT_MAGENTA);
  tft.setCursor(0, 42);
  tft.print("Point ");
  tft.print(currentwaypoint+1);

  tft.setCursor(0, 58);
  tft.setTextSize(1);
  tft.print("Lt:");
  tft.print (waypoint[currentwaypoint][0]);
  tft.print(" Lg:");
  tft.print (waypoint[currentwaypoint][1]);

  tft.setTextSize(2);
  tft.setTextColor(TFT_GREEN);
  tft.setCursor(0, 69);
  tft.print ("Dst");
  double dist = round (gps.distanceBetween (gps.location.lat(), gps.location.lng(), waypoint[currentwaypoint][0], waypoint[currentwaypoint][1]));
  if (dist < 999999){ tft.print (" "); tft.print ((uint16_t)dist);}
  else tft.print (">999999");
  tft.setCursor(0, 85);
  tft.print ("Crs ");
  double cours = round (gps.courseTo (gps.location.lat(), gps.location.lng(), waypoint[currentwaypoint][0], waypoint[currentwaypoint][1])); 
  tft.print ((uint16_t)cours);
  tft.setCursor(0, 101);
  tft.print ("Dir ");
  tft.print(gps.cardinal(cours));
  
  tft.setTextSize(1);
  tft.setTextColor(TFT_RED);
  tft.setCursor(0, 120);
  if (gpsdata.coordage > GPSdatatimeout) {
    tft.fillRect(0, 120, 128, 8, TFT_BLACK);
    tft.print(" GPS connection lost!");}
 }
}


boolean esp_eeprom_save(){
  EEPROM.put (0, waypoint);
  return (EEPROM.commit());
}


void esp_eeprom_load(){
    EEPROM.get(0, waypoint);
}


uint8_t getKeys() { return (~mcp.readGPIOAB() & 255); }

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
  tone (SOUNDpin, 800, 20);
  pixels.setPixelColor(0, pixels.Color(0,0,20));
  pixels.show();  
if (displaymode == 0){ 
  if ((bt&PAD_UP || bt&PAD_RIGHT)  && currentwaypoint != 4) currentwaypoint++;
  if ((bt&PAD_DOWN || bt&PAD_LEFT) && currentwaypoint != 0) currentwaypoint--; 
  if (bt&PAD_ACT){
      esp_eeprom_needsaving++;
      waypoint[currentwaypoint][0] = gpsdata.lat;
      waypoint[currentwaypoint][1] = gpsdata.lng;
      tft.fillRect(0, 104, 128, 10, TFT_BLACK);
      tft.setTextSize(1);
      tft.setTextColor(TFT_MAGENTA);
      tft.setCursor(0, 104);
      tft.print ("  waypoint ");
      tft.print (currentwaypoint+1);
      tft.print (" stored");
      tone (SOUNDpin, 300, 100);
      smartDelay (2500);
   }
   if (bt&PAD_ESC) {
     displaymode++;
     if (displaymode >1)  displaymode = 0;
     tft.fillScreen(TFT_BLACK);}
}

else{
   if (bt&PAD_ESC) {
     displaymode++;
     if (displaymode >1)  
     displaymode = 0; 
     tft.fillScreen(TFT_BLACK);
     }
   if (((bt&PAD_UP) || (bt&PAD_RIGHT))  && currentwaypoint != 4) currentwaypoint++;
   if (((bt&PAD_DOWN) || (bt&PAD_LEFT)) && currentwaypoint != 0) currentwaypoint--;    
}

  while (!getKeys()) smartDelay(0);
  drawled();
  drawtft();
  delay(200); 
}


static void smartDelay(unsigned long ms){
  static unsigned long start;
  start = millis();
  do {
    while (ss.available())
      gps.encode(ss.read());
      delay (5);
  } while (millis() - start < ms);
}



void setup(){
  Serial.begin(115200); //serial init
  ss.begin(GPSBaud);   //software serial init
  delay (100);
  WiFi.mode(WIFI_OFF); // to safe some battery power

//LED init
  pinMode(LEDpin, OUTPUT);
  pixels.begin();
  delay (100);
  pixels.setPixelColor(0, pixels.Color(0,0,0));
  pixels.show();

//MCP23017 init
  mcp.begin(MCP23017address);
  delay(100);
  for (int i = 0; i < 8; ++i) {
    mcp.pinMode(i, INPUT);
    mcp.pullUp(i, HIGH);}

 //TFT init     
  mcp.pinMode(csTFTMCP23017pin, OUTPUT);
  mcp.digitalWrite(csTFTMCP23017pin, LOW);
  tft.begin();
  delay(100);
  tft.setRotation(0);
  tft.fillScreen(TFT_BLACK);

//draw ESPboylogo  
  tft.drawXBitmap(30, 24, ESPboyLogo, 68, 64, TFT_YELLOW);
  tft.setTextSize(1);
  tft.setTextColor(TFT_YELLOW);
  tft.setCursor(24,102);
  tft.print ("GPS navigator");
    
//global vars init  
  memset (waypoint, sizeof(waypoint), 0);

//sound init and test
  pinMode(SOUNDpin, OUTPUT);
  tone(SOUNDpin, 200, 100); 
  delay(100);
  tone(SOUNDpin, 100, 100);
  delay(100);
  noTone(SOUNDpin);

//BAT voltage measure init
  pinMode(A0, INPUT);
  pinMode (RXpin, INPUT_PULLUP);

//check OTA
 if (getKeys()&PAD_ACT || getKeys()&PAD_ESC) OTAobj = new ESPboyOTA(&tft, &mcp);

//load last waypoint state from eeprom
  EEPROM.begin(sizeof (waypoint));
  esp_eeprom_load();

//clear TFT
  delay(2000);
  tft.fillScreen(TFT_BLACK);
}



void loop(){
 static long count;
   if (esp_eeprom_needsaving) {
     esp_eeprom_save();
     esp_eeprom_needsaving = 0;
   }
   while (ss.available()) gps.encode(ss.read());
   fillgpsstruct(); 
   uint8_t bt=getKeys();
   if (bt) runButtonsCommand(bt);
   if (millis() > count + 3000){
     count = millis();
     drawtft(); 
     drawled();
    }
}

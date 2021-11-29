#include <Arduino.h>
#include <FastLED.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <FS.h>
#include <ESP8266FtpServer.h>
#include <pass.h>


/************* Led Settings ***********/
const byte led = 2;       // Led pin
const byte NUM_LEDS = 1;  // Number of LEDs
const byte DATA_PIN = D3; // Data pin for ws2812
CRGB leds[NUM_LEDS];      // Define the array of leds


/************ Wi-Fi Settings **********/
IPAddress ip(10,1,1,222); // Static IP
IPAddress gateway(10,1,1,222);
IPAddress subnet(255, 255, 255, 0);


/************ Ftp Settings ************/
FtpServer ftpSrv;


/********* WebServer Settings *********/
ESP8266WebServer HTTP(80);


/************* Variables ***********/
bool state = 0;


String led_switch() {
  if (state == 1)
  {
    state = 0;
    Serial.println("on");
    leds [1] = CRGB::White;
    FastLED.show();
  } else {
    state = 1;
    Serial.println("off");
    leds[1] = CRGB::Red;
    FastLED.show();
  }

  digitalWrite(led, state);
  return String(state);
}


String led_status() {
  if (digitalRead(led)) {
    state = 1;
  } else {
  state = 0;
  }
  return String(state);
}


String getContentType(String filename) {
  if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".css")) return "text/css";
  else if (filename.endsWith(".js")) return "application/javascript";
  return "text/plain";
}


bool handleFileRead(String path) {
  if (path.endsWith("/")) path += "index.html";
  String contentType = getContentType(path);
  if (SPIFFS.exists(path)) {
    File file = SPIFFS.open(path, "r");
    size_t sent = HTTP.streamFile(file, contentType);
    file.close();
    return true;
  }
  return false;
}


void setup() {

  pinMode(led, OUTPUT);
  Serial.begin(9600);

  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);


  /****** Wi-Fi connection ******/
  WiFi.begin(ssid, password);
  WiFi.config(ip, gateway, subnet);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("connected");

  SPIFFS.begin();
  ftpSrv.begin("relay", "relay");
  HTTP.begin();


  // Processing HTTP requests
  HTTP.on("/led_switch", []() {HTTP.send(200, "text/plain", led_switch()); });  // Turning on the flashlight
  HTTP.on("/led_status", []() {HTTP.send(200, "text/plain", led_status()); });  // Status flashlight

  HTTP.onNotFound([]() {
    if (!handleFileRead(HTTP.uri()))
      HTTP.send(404, "text/plain", "Not Found");
  });
}


void loop() {
  HTTP.handleClient();  // HTTP handler
  ftpSrv.handleFTP();   // FTP handler
}

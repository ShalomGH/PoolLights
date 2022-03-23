#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <FS.h>
#include <ESP8266FtpServer.h>
#include <pass.h>
#include <Wire.h>


/************* Led Settings ***********/
const byte led = 2;       // Led pin


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


void sendFunction(byte mode, byte red, byte green, byte blue) {
  Wire.beginTransmission(0x04);
  Wire.write(mode);
  Wire.write(red);
  Wire.write(green);
  Wire.write(blue);
  Wire.endTransmission();
  Serial.println("finish");
}


//effects requst handler
void effectHandler() {
    if (HTTP.argName(0) == "effect" && HTTP.argName(1) == "param2") {   //check for correct params
      byte effect = HTTP.arg(0).toInt();
      Serial.print("effect = ");
      Serial.println(effect);

      byte param1 = HTTP.arg(1).toInt();
      Serial.print("param2 = ");
      Serial.println(param1);
      HTTP.send(200);
      sendFunction(effect, effect, param1, param1);
    }
    else {  // bad requests handler
      HTTP.send(400, "text/plain", "Bad Request");
      Serial.println("Bad Request");
    }
}


// switch function
String led_switch() {
  if (state == 1){
    state = 0;
    sendFunction(1, 1, 1, 1);
    Serial.println("on");
  } else {
    state = 1;
    sendFunction(0, 0, 0, 0);
    Serial.println("off");
  }
  digitalWrite(led, state);
  return String(state);
}


// on/off status checker
String led_status() {
  if (digitalRead(led)) {
    state = 1;
  } else {
  state = 0;
  }
  return String(state);
}


// file handler
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

  Wire.begin(D1, D2); // initialize wire connection

  /****** Wi-Fi connection ******/
  WiFi.begin(ssid, password);
  WiFi.config(ip, gateway, subnet);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("connected");

  //
  SPIFFS.begin();
  ftpSrv.begin("relay", "relay");
  HTTP.begin();


  // Processing HTTP requests
  HTTP.on("/led_switch", []() {HTTP.send(200, "text/plain", led_switch()); });  // Turning on the flashlight
  HTTP.on("/led_status", []() {HTTP.send(200, "text/plain", led_status()); });  // Status flashlight
  HTTP.on("/effect", effectHandler); //Associate the handler function to the path

  HTTP.onNotFound([]() {
    if (!handleFileRead(HTTP.uri()))
      HTTP.send(404, "text/plain", "Not Found");
  });
}


void loop() {
  HTTP.handleClient();  // HTTP handler
  ftpSrv.handleFTP();   // FTP handler
}
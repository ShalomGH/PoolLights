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
bool pos = 0;


void sendFunction(byte mode, byte params) {
  Wire.beginTransmission(0x04);
  Wire.write(mode);
  Wire.write(params);
  Wire.write(pos);
  Wire.endTransmission();
  Serial.println("finish");
}


//effects requst handler
void effectHandler() {
    if (HTTP.argName(0) == "mode" && HTTP.argName(1) == "param") {   //check for correct params
      byte modes = HTTP.arg(0).toInt();
      Serial.print("mode = ");
      Serial.println(modes);

      byte param = HTTP.arg(1).toInt();
      Serial.print("param = ");
      Serial.println(param);
      HTTP.send(200);
      if (modes != 0 && pos == 0) {
        pos=1;
      }
      if (modes == 0 && pos == 0) {
        Serial.println("modes == 0 && pos == 0");
        pos=1;
      } else if(modes == 0 && pos == 1){
        pos = 0;
      }
      Serial.print("pos = ");
      Serial.println(pos);
      sendFunction(modes, param);
    }
    else {  // bad requests handler
      HTTP.send(400, "text/plain", "Bad Request");
      Serial.println("Bad Request");
    }
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
  pos = 0;
  digitalWrite(led, pos);

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


  SPIFFS.begin();
  ftpSrv.begin("relay", "relay");
  HTTP.begin();


  // Processing HTTP requests
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

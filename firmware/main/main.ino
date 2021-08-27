#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <FS.h>
#include <ESP8266FtpServer.h>

const byte led = 2; // Пин подключения светодиода


const char* ssid = "password";
const char* password = "password";
IPAddress ip(10,1,1,222); //статический IP
IPAddress gateway(10,1,1,222);
IPAddress subnet(255, 255, 255, 0);


ESP8266WebServer HTTP(80);
FtpServer ftpSrv;

void setup() {
  pinMode(led, OUTPUT);
  Serial.begin(9600);

  WiFi.begin(ssid, password);
  WiFi.config(ip, gateway, subnet);
  Serial.println("");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  SPIFFS.begin();
  HTTP.begin();
  ftpSrv.begin("relay", "relay");

  Serial.println(WiFi.softAPIP()); // Выводим локальный IP-адрес ESP8266
  Serial.println("\n");

  // Обработка HTTP-запросов
  HTTP.on("/led_switch", []() {
    HTTP.send(200, "text/plain", led_switch());
  });
  HTTP.on("/led_status", []() {
    HTTP.send(200, "text/plain", led_status());
  });
  HTTP.onNotFound([]() {
    if (!handleFileRead(HTTP.uri()))
      HTTP.send(404, "text/plain", "Not Found");
  });
}

void loop() {
  HTTP.handleClient(); // Обработчик HTTP-событий
  ftpSrv.handleFTP(); // Обработчик FTP-соединений
}

String led_switch() { // Функция переключения светодиода
  byte state;
  if (digitalRead(led))
    state = 0;
  else
    state = 1;
  digitalWrite(led, state);
  return String(state);
}

String led_status() { // Функция для определения текущего статуса светодиода
  byte state;
  if (digitalRead(led))
    state = 1;
  else
    state = 0;
  return String(state);
}

bool handleFileRead(String path) { // Функция работы с файловой системой
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

String getContentType(String filename) { // Функция, возвращающая необходимый заголовок типа содержимого в зависимости от расширения файла
  if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".css")) return "text/css";
  else if (filename.endsWith(".js")) return "application/javascript";
  else if (filename.endsWith(".png")) return "image/png";
  else if (filename.endsWith(".jpg")) return "image/jpeg";
  else if (filename.endsWith(".gif")) return "image/gif";
  else if (filename.endsWith(".ico")) return "image/x-icon";
  return "text/plain";
}
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <FS.h>

const char* VERSION = "1.0.0-SNAPSHOT";
const int MODE_PIN = 0; // GPIO0
const char* SETTINGS_FILE_PATH = "/wifi_settings.txt";
const String AP_PASS = "thereisnospoon";

IPAddress ip(192, 168, 128, 1);
IPAddress subnet(255, 255, 255, 0);

boolean isWebServer;

WiFiServer tcpServer(80);
ESP8266WebServer webServer(8080);

void handleRootGet() {
  String html = "";
  html += "<h1>Wi-Fi Settings</h1>";
  html += "<form method='post'>";
  html += "  <input type='text' name='ssid' placeholder='ssid'><br>";
  html += "  <input type='text' name='pass' placeholder='pass'><br>";
  html += "  <input type='submit'><br>";
  html += "</form>";
  webServer.send(200, "text/html", html);
}

void handleRootPost() {
  String ssid = webServer.arg("ssid");
  String pass = webServer.arg("pass");

  File f = SPIFFS.open(SETTINGS_FILE_PATH, "w");
  if (!f) {
    Serial.println("--File open failed");
  } else {
    f.println(ssid);
    f.println(pass);
    f.close();
  }
  Serial.println("--SSID: " + ssid);
  Serial.println("--PASS: " + pass);

  String html = "";
  html += "<h1>WiFi Settings</h1>";
  html += ssid + "<br>";
  html += pass + "<br>";
  webServer.send(200, "text/html", html);
}

void setupWebServer() {
  byte mac[6];
  WiFi.macAddress(mac);
  String ssid = "";
  for (int i = 0; i < 6; i++) {
    ssid += String(mac[i], HEX);
  }
  Serial.println("--AP_SSID: " + ssid);
  Serial.println("--AP_PASS: " + AP_PASS);

  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(ip, ip, subnet);
  WiFi.softAP(ssid.c_str(), AP_PASS.c_str());

  webServer.on("/", HTTP_GET, handleRootGet);
  webServer.on("/", HTTP_POST, handleRootPost);
  webServer.begin();
  Serial.println("--HTTP server started.");
}

void setupTcpServer() {
  String ssid = "";
  String pass = "";
  File f = SPIFFS.open(SETTINGS_FILE_PATH, "r");
  if (!f) {
    Serial.println("--File open failed");
  } else {
    ssid = f.readStringUntil('\n');
    pass = f.readStringUntil('\n');
    f.close();
  }

  ssid.trim();
  pass.trim();

  Serial.println("--SSID: " + ssid);
  Serial.println("--PASS: " + pass);

  WiFi.disconnect();
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), pass.c_str());

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print("--");
  }

  Serial.println();
  Serial.println("--WiFi connected");
  Serial.print("--IP address: ");
  Serial.println(WiFi.localIP());
  tcpServer.begin();
}

void setup() {
  Serial.begin(19200);
  delay(1000);

  SPIFFS.begin();

  pinMode(MODE_PIN, INPUT);
  isWebServer = digitalRead(MODE_PIN) == 0;
  if (isWebServer) {
    setupWebServer();
  } else {
    setupTcpServer();
  }
}

void loop(){
  if (isWebServer) {
    webServer.handleClient();
  } else {
    WiFiClient client = tcpServer.available();
    if (client) {
      Serial.println("--New Client.");
      String currentLine = "";
      String tmp;
      while (client.connected()) {
        if (client.available()) {
          char c = client.read();
          if (c == '\n') {
            currentLine = tmp;
            tmp = "";
          } else {
            tmp += c;
          }

          if (currentLine != "") {
            Serial.println(currentLine);
            currentLine = "";
          }
        }
      }

      client.stop();
      Serial.println("--Client Disconnected.");
    }
  }
}

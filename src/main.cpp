#include <Arduino.h>
#include "FS.h"

/**
 * jalankan module Wifi.h dan Async pattern jika mengunakan board ESP32
 * akan tetapi jika menggunakan board esp8266 maka jalankan module espasync untuk esp8266
 * jalankan module Espasync web server
 * jalankan module spi, adafruit gfx dan ssd oled lcd screen
 */
#ifdef ESP32
#include <WiFi.h>
#include <AsyncTCP.h>
#else
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#endif
#include <ESPAsyncWebServer.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

/**
 * define layar pada oled dan declare display oled
 */
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

Adafruit_SSD1306 oled_display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

/**
 * declare var untuk nama wifi,password
 * declare var untuk login auth berupa username dan password
 * declare var untuk status pada saklar lampu
 * declare var untuk input pin pada saklar otomatisasi pada lampu
 */
const char *ssid = "SkripsiIOT";
const char *password = "1234567890";

const char *http_username = "admin";
const char *http_password = "skripsi";

const char *PARAM_INPUT_1 = "state";

const int output = 26;

// Init/jalankan module pattern async web server
AsyncWebServer server(80);

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>ESP Web Server</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    html {font-family: Arial; display: inline-block; text-align: center;}
    h2 {font-size: 2.6rem;}
    body {max-width: 600px; margin:0px auto; padding-bottom: 10px;}
    .switch {position: relative; display: inline-block; width: 120px; height: 68px} 
    .switch input {display: none}
    .slider {position: absolute; top: 0; left: 0; right: 0; bottom: 0; background-color: #ccc; border-radius: 34px}
    .slider:before {position: absolute; content: ""; height: 52px; width: 52px; left: 8px; bottom: 8px; background-color: #fff; -webkit-transition: .4s; transition: .4s; border-radius: 68px}
    input:checked+.slider {background-color: #2196F3}
    input:checked+.slider:before {-webkit-transform: translateX(52px); -ms-transform: translateX(52px); transform: translateX(52px)}
  </style>
</head>
<body>
  <h2>ESP Web Server</h2>
  <button onclick="logoutButton()">Logout</button>
  <p>Ouput - GPIO 2 - State <span id="state">%STATE%</span></p>
  %BUTTONPLACEHOLDER%
<script>function toggleCheckbox(element) {
  var xhr = new XMLHttpRequest();
  if(element.checked){ 
    xhr.open("GET", "/update?state=1", true); 
    document.getElementById("state").innerHTML = "ON";  
  }
  else { 
    xhr.open("GET", "/update?state=0", true); 
    document.getElementById("state").innerHTML = "OFF";      
  }
  xhr.send();
}
function logoutButton() {
  var xhr = new XMLHttpRequest();
  xhr.open("GET", "/logout", true);
  xhr.send();
  setTimeout(function(){ window.open("/logged-out","_self"); }, 1000);
}
</script>
</body>
</html>
)rawliteral";

const char logout_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
</head>
<body>
  <p>Logged out or <a href="/">return to homepage</a>.</p>
  <p><strong>Note:</strong> close all web browser tabs to complete the logout process.</p>
</body>
</html>
)rawliteral";
String outputState()
{
  if (digitalRead(output))
  {
    return "checked";
  }
  else
  {
    return "";
  }
  return "";
}
// Replaces placeholder with button section in your web page
String processor(const String &var)
{
  // Serial.println(var);
  if (var == "BUTTONPLACEHOLDER")
  {
    String buttons = "";
    String outputStateValue = outputState();
    buttons += "<p><label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" id=\"output\" " + outputStateValue + "><span class=\"slider\"></span></label></p>";
    return buttons;
  }
  if (var == "STATE")
  {
    if (digitalRead(output))
    {
      return "ON";
    }
    else
    {
      return "OFF";
    }
  }
  return String();
}

void setup()
{
  // Serial port for debugging purposes
  Serial.begin(115200);

  // pin mode untuk saklar/relay
  pinMode(output, OUTPUT);
  digitalWrite(output, LOW);

  // pin mode untuk lampu esp32
  pinMode(LED_BUILTIN, OUTPUT);

  /**
   * display setup
   */
  if (!oled_display.begin(SSD1306_SWITCHCAPVCC, 0x3c))
  {
    Serial.println(F("Oled do not detected"));
    for (;;)
      ;
  }
  delay(2000);
  oled_display.clearDisplay();
  oled_display.setTextSize(1);
  oled_display.setTextColor(WHITE);
  oled_display.setCursor(0, 10);
  oled_display.println("Hello Skripsi IOT");
  oled_display.display();

  /**
   * Connect to Wi-Fi Sebagai (client side)
   */

  // WiFi.begin(ssid, password);
  // while (WiFi.status() != WL_CONNECTED)
  // {
  //   delay(1000);
  //   Serial.println("Connecting to WiFi..");
  // }
  // Print ESP Local IP Address
  // Serial.println(WiFi.localIP());

  /**
   * Connect Wi-Fi Sebagai (Access Point)
   */

  Serial.println("Setting AP (Access Point) Skripsi IOT…");
  // Remove the password parameter, if you want the AP (Access Point) to be open
  WiFi.softAP(ssid, password);

  IPAddress IP = WiFi.softAPIP();
  digitalWrite(LED_BUILTIN, HIGH);
  Serial.print("AP IP address: ");
  Serial.println(IP);

  /**
   *
   * Routes(web) sebagai endpoint dari sebuah fitur
   */
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    if(!request->authenticate(http_username, http_password))
      return request->requestAuthentication();
    request->send_P(200, "text/html", index_html, processor); });

  server.on("/logout", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(401); });

  server.on("/logged-out", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send_P(200, "text/html", logout_html, processor); });

  // Send a GET request to <ESP_IP>/update?state=<inputMessage>
  server.on("/update", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    if(!request->authenticate(http_username, http_password))
      return request->requestAuthentication();
    String inputMessage;
    String inputParam;
    // GET input1 value on <ESP_IP>/update?state=<inputMessage>
    if (request->hasParam(PARAM_INPUT_1)) {
      inputMessage = request->getParam(PARAM_INPUT_1)->value();
      inputParam = PARAM_INPUT_1;
      digitalWrite(output, inputMessage.toInt());
    }
    else {
      inputMessage = "No message sent";
      inputParam = "none";
    }
    Serial.println(inputMessage);
    request->send(200, "text/plain", "OK"); });

  // Start server
  server.begin();
}

void loop()
{
}
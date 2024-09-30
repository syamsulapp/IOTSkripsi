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
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

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

const char *PARAM_INPUT_2 = "status";

const int output = 26;
const int output_sklar_2 = 26;

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
  <h2>Otomatisasi Lampu Dengan ESP-32 Dan Relay</h2>
  <button onclick="logoutButton()">Logout</button>
  <p>Kondisi - Lampu 1 - State <span id="state">%STATE%</span></p>
  %BUTTONPLACEHOLDER%
  <p>Kondisi - Lampu 2 - State <span id="lampu2">%LAMPU2%</span></p>
  %BUTTONLAMPU2PLACEHOLDER%
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

function toggleButton2(element) {
  var xhrButton2 = new XMLHttpRequest();
  if(element.checked){
    xhrButton2.open("GET", "/saklar?status=1", true);
    document.getElementById("lampu2").innerHTML = "ON";  
  } else {
    xhrButton2.open("GET", "/saklar?status=0", true);
    document.getElementById("lampu2").innerHTML = "OFF";      
  }
  xhrButton2.send();
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

/**
 * method output state digunakan untuk mengetahui status tombol pada lampu pijar
 */
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

String ButtonLampu2State()
{
  if (digitalRead(output_sklar_2))
  {
    return "checked";
  }
  else
  {
    return "";
  }
  return "";
}

/**
 * methods process digunakan untuk mengeksekusi saklar pada lampu
 */
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

  // lampu 2
  if (var == "BUTTONLAMPU2PLACEHOLDER")
  {
    String ButtonLampu2 = "";
    String ButtonLampu2Value = ButtonLampu2State();
    ButtonLampu2 += "<p><label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleButton2(this)\" id=\"output_sklar_2\" " + ButtonLampu2Value + "><span class=\"slider\"></span></label></p>";
    return ButtonLampu2;
  }
  if (var == "LAMPU2")
  {
    if (digitalRead(output_sklar_2))
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
  // Baude rate proses
  Serial.begin(115200);

  // pin mode untuk saklar/relay
  pinMode(output, OUTPUT);
  digitalWrite(output, LOW);

  // pinmode saklar 2
  pinMode(output_sklar_2, OUTPUT);
  digitalWrite(output_sklar_2, LOW);

  // pin mode untuk lampu esp32
  pinMode(LED_BUILTIN, OUTPUT);

  /**
   * setup layar oled,digunakan untuk menampilkan tulisan hello skripsi IOT
   * dan alamat dari fitur aplikasi
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
  oled_display.setCursor(0, 5);
  oled_display.println("Hello Skripsi IOT");

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
  oled_display.setCursor(0, 30);
  oled_display.print("AP IP address: ");
  oled_display.setCursor(0, 50);
  oled_display.println(IP);
  oled_display.display();
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

  // Send a GET request to <ESP_IP>/saklar?status=<InputValue>
  server.on("/saklar", HTTP_GET, [](AsyncWebServerRequest *request)
            {
              if (!request->authenticate(http_username, http_password))
                return request->requestAuthentication();
                String InputValue;
                String InputParam;
                    // GET input1 value on <ESP_IP>/saklar?status=<InputValue>
                if(request->hasParam(PARAM_INPUT_2)){
                  InputValue = request->getParam(PARAM_INPUT_2)->value();
                  InputParam = PARAM_INPUT_2;
                  digitalWrite(output_sklar_2,InputValue.toInt());
                }else {
                  InputValue = "no request send";
                  InputParam = "none";
                } 
                Serial.println(InputValue);
                request->send(200,"text/plain","OK"); });
  // Start server
  server.begin();
}

void loop()
{
}
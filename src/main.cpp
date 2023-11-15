#include <Arduino.h>
#include "FS.h"

#include <WiFi.h>

// Replace with your network credentials
const char *ssid = "IOTSkripsi";
const char *password = "12345678910";

// milis
unsigned long currentTime = millis();
unsigned long previousTime = 0;
const long timeoutTime = 2000;

// Set web server port number to 80
WiFiServer server(80);

// Variable to store the HTTP request
String header;

// Auxiliar variables to store the current output state
String Lamp1State = "off";

// Assign output variables to GPIO pins
const int Lamp1 = 26;

void setup()
{
  Serial.begin(115200);
  // Initialize the output variables as outputs
  pinMode(Lamp1, OUTPUT);
  // Set outputs to LOW
  digitalWrite(Lamp1, LOW);

  // Connect to Wi-Fi network with SSID and password
  Serial.print("Setting AP (Access Point)…");
  // Remove the password parameter, if you want the AP (Access Point) to be open
  WiFi.softAP(ssid, password);

  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);

  server.begin();
}

void loop()
{
  WiFiClient client = server.available();
  if (client)
  {
    currentTime = millis();
    previousTime = currentTime;
    Serial.println("New Client.");
    String currentLine = "";
    while (client.connected() && currentTime - previousTime <= timeoutTime)
    {
      currentTime = millis();
      if (client.available())
      {
        char c = client.read();
        Serial.write(c);
        header += c;
        if (c == '\n')
        {
          if (currentLine.length() == 0)
          {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();

            if (header.indexOf("GET /26/off") >= 0)
            {
              Serial.println("Lampu Mati");
              Lamp1State = "off";
              digitalWrite(Lamp1, HIGH);
            }
            else if (header.indexOf("GET /26/on") >= 0)
            {
              Serial.println("Lampu Menyala");
              Lamp1State = "on";
              digitalWrite(Lamp1, LOW);
            }

            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
            client.println(".button { background-color: #3DE31B; border: none; color: black; padding: 16px 40px;");
            client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
            client.println(".button2 {background-color: #DB240F;}</style></head>");

            client.println("<body><h1>ESP32 IOT Web Server Control Lamp</h1>");
            client.println("<p>Lamp 1 - Current Condition <b>" + Lamp1State + "</b></p>");
            if (Lamp1State == "off")
            {
              client.println("<p><a href=\"/26/on\"><button class=\"button\">ON</button></a></p>");
            }
            else
            {
              client.println("<p><a href=\"/26/off\"><button class=\"button button2\">OFF</button></a></p>");
            }
            client.println("</body></html>");
            client.println();
            break;
          }
          else
          {
            currentLine = "";
          }
        }
        else if (c != '\r')
        {
          currentLine += c;
        }
      }
    }
    header = "";
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }
}
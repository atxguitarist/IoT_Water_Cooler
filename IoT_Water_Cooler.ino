#include <ESP8266WiFi.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <ESP8266WebServer.h>

// Replace with your network credentials
const char* ssid = "xxxxxxxxxxxxxx";
const char* password = "xxxxxxxxxxxxxxx";
IPAddress staticIP(192, 168, 1, 10); // Example static IP
IPAddress gateway(192, 168, 1, 1);    // Gateway (usually your router IP)
IPAddress subnet(255, 255, 255, 0);   // Subnet Mask
IPAddress dns(8, 8, 8, 8);            // DNS server (Google DNS used here)

// Data wire for DS18B20 is connected to GPIO2 (D4 on ESP8266)
#define ONE_WIRE_BUS 2

// GPIO where the relay is connected
const int relayPin = 5; // Example GPIO5 (D1 on ESP8266)

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
ESP8266WebServer server(80);

// Temperature thresholds
float minTemp = 38.0;
float maxTemp = 48.0;

void setup() {
  Serial.begin(115200);
  sensors.begin();
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, LOW); // Assuming LOW turns the relay OFF
  
  WiFi.config(staticIP, gateway, subnet, dns);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi. IP address: ");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.on("/setTemperature", handleTemperatureSet);
  server.begin();
}

void loop() {
  sensors.requestTemperatures();
  controlRelay(sensors.getTempFByIndex(0));
  server.handleClient();
}

void controlRelay(float fahrenheitTemp) {
  Serial.print("Current temperature is: ");
  Serial.println(fahrenheitTemp);

  if (fahrenheitTemp >= maxTemp) {
    digitalWrite(relayPin, HIGH); // Turns the relay ON
  } else if (fahrenheitTemp <= minTemp) {
    digitalWrite(relayPin, LOW); // Turns the relay OFF
  }
}

void handleRoot() {
  String html = "<html><body>";
  html += "<h1>IoT Water Cooler Settings</h1>";
  html += "<p>Set minimum and maximum temperatures:</p>";
  html += "<form action=\"/setTemperature\" method=\"POST\">";
  html += "Min Temp (&deg;F): <input type=\"number\" name=\"minTemp\" value=\"" + String(minTemp) + "\"><br>";
  html += "Max Temp (&deg;F): <input type=\"number\" name=\"maxTemp\" value=\"" + String(maxTemp) + "\"><br>";
  html += "<input type=\"submit\" value=\"Update\">";
  html += "</form>";
  html += "<p>Current water temperature: " + String(sensors.getTempFByIndex(0)) + " &deg;F</p>";
  html += "</body></html>";

  server.send(200, "text/html", html);
}

void handleTemperatureSet() {
  if (server.hasArg("minTemp") && server.hasArg("maxTemp")) {
    minTemp = server.arg("minTemp").toFloat();
    maxTemp = server.arg("maxTemp").toFloat();
  }
  server.sendHeader("Location", "/", true);
  server.send(303); // HTTP response code 303 (See Other) to redirect
}

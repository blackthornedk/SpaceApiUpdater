/*
 * Relay Shield - SpaceApi Updater
 * Turns on the relay, if the OSAA SpaceApi Status is 'open'.
 *
 * Relay Shield transistor closes relay when D1 is HIGH
 */
/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <black@blackthorne.dk> wrote this file. As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return Jacob V. Rasmussen
 * ----------------------------------------------------------------------------
 */

#include <ESP8266WiFi.h>
#include <Arduino.h>
#include <ArduinoJson.h>

const int relayPin = D1;
const long interval = 5000;  // pause for five seconds

const char* ssid     = "WIFIName";
const char* password = "WIFIPass";

const char* host = "spaceapi.osaa.dk";
const char* url = "/status/json";

void setup() {
  Serial.begin(9600);
  delay(10);

  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");  
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  pinMode(relayPin, OUTPUT);
}

void loop() {
  delay(interval);              // pause

  Serial.print("connecting to ");
  Serial.println(host);
  
  // Use WiFiClient class to create TCP connections
  WiFiClient client;
  const int httpPort = 80;
  if (!client.connect(host, httpPort)) {
    Serial.println("connection failed");
    return;
  }
  
  Serial.print("Requesting URL: ");
  Serial.println(url);
  
  // This will send the request to the server
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" + 
               "Connection: close\r\n\r\n");
  unsigned long timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 5000) {
      Serial.println(">>> Client Timeout !");
      client.stop();
      return;
    }
  }

  bool found = false;
  int next = 0;
  
  // Read all the lines of the reply from server and print them to Serial
  while(client.available()){
    String line = client.readStringUntil('\r');
    //Serial.print(line);
    line.trim();
    if (line.length() == 0) {
      found = true;
    }
    
    if (found) {
      next++;
      if (next == 2) {
        Serial.println(line);

        // {
        //   "api":"0.13",
        //   "space":"Open Space Aarhus",
        //   "logo":"http:\/\/osaa.dk\/wp-uploads\/wiki_logo.png",
        //   "url":"http:\/\/osaa.dk",
        //   "location": {
        //     "address":"Open Space Aarhus, Katrinebjergvej 105, 8200 Aarhus N, Denmark",
        //     "lon":10.1872507,
        //     "lat":56.1732918
        //   },
        //   "contact": {
        //     "twitter":"@openspaceaarhus",
        //     "email":"info@osaa.dk",
        //     "issue_mail":"robotto@osaa.dk",
        //     "irc":"irc:\/\/irc.freenode.net\/#osaa",
        //     "ml":"openspaceaarhus@googlegroups.com"
        //   },
        //   "issue_report_channels":["issue_mail"],
        //   "state": {
        //     "open":true
        //   },
        //   "projects":["http:\/\/github.com\/openspaceaarhus"],
        //   "cache": {
        //     "schedule":"m.02"
        //   },
        //   "ext_gist":"91e49416e7595bc1517b"
        // }

        const size_t BUFFER_SIZE = 
            JSON_OBJECT_SIZE(11)    // 11 elements in root
            + JSON_OBJECT_SIZE(3)   // 3 elements in location
            + JSON_OBJECT_SIZE(5)   // 5 elements in contact
            + JSON_OBJECT_SIZE(1)   // 1 element in state
            + JSON_OBJECT_SIZE(1)   // 1 element in cache
            + JSON_OBJECT_SIZE(2)   // 2 elements in array
            + JSON_OBJECT_SIZE(100);// above calculation failed, we have enough RAM, so make a bit more room.
            
        StaticJsonBuffer<BUFFER_SIZE> jsonBuffer;
        JsonObject& root = jsonBuffer.parseObject(line);

        if (!root.success()) {
          Serial.println("parseObject() failed");
          return;
        }
        Serial.println("Parse Complete");

        bool state = root["state"]["open"];

        if (!state) {
            Serial.println("Space is OPEN");
            digitalWrite(relayPin, HIGH); // turn on relay with voltage HIGH
        } else {
            Serial.println("Spave is CLOSED");
            digitalWrite(relayPin, LOW);  // turn off relay with voltage LOW
        }
      }
    }
  }
  
  Serial.println();
  Serial.println("closing connection");
}

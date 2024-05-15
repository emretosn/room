#include <DHT.h>
#include <DHT_U.h>
#include "MQ135.h"
#include <SoftwareSerial.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ESP8266HTTPClient.h>
#include "secrets.h"

#define BUZZER D0
#define DHTPIN D1
#define DHTTYPE DHT11

#define ANALOGPIN A0
MQ135 gasSensor = MQ135(ANALOGPIN);

// Network Credentials
const char* ssid     = SECRET_SSID;
const char* password = SECRET_PASS;

// Supabase Credentials
String api_url       = API_URL;
String api_key       = API_KEY;
String table_name    = TABLE_NAME;
const int httpsPort  = 443;

// Interval to send to database in seconds
int sendinginterval = 10;

HTTPClient https;
WiFiClientSecure client;
DHT dht(DHTPIN, DHTTYPE);

// Sensor values
float t;
float h;
float hic;
float rzero;

const int numReadings           = 3;
const int numSensors            = 4;
float sensorTotal[numSensors]   = { 0 };
float sensorAverage[numSensors] = { 0 };
int currentIndex                = 0;

void setup() {
    client.setInsecure();

    Serial.begin(115200);

    dht.begin();
    pinMode(BUZZER, OUTPUT);

    Serial.print("Connecting to ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.println("WiFi connected.");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
}

void loop() {

    t = dht.readTemperature();
    h = dht.readHumidity();
    hic = dht.computeHeatIndex(t, h, false);

    rzero = gasSensor.getRZero();

    // Preprocessing
    for (int i = 0; i < numSensors; i++) {
        sensorTotal[i] += (i == 0) ? t : ((i == 1) ? h : ((i == 2) ? hic : rzero));
    }

    currentIndex++;

    if (currentIndex == numReadings) {
        for (int i = 0; i < numSensors; i++) {
            sensorAverage[i] = sensorTotal[i] / numReadings;
            sensorTotal[i] = 0;
        }

        if (sensorAverage[3] < 3) {
            buzzerBeep(3);
        }

        // Check the connection and send data
        if (WiFi.status() == WL_CONNECTED) {
            // Send the a post request to the server
            https.begin(client,api_url+"/rest/v1/"+table_name);
            https.addHeader("Content-Type", "application/json");
            https.addHeader("Prefer", "return=representation");
            https.addHeader("apikey", api_key);
            https.addHeader("Authorization", "Bearer " + api_key);
            int httpCode = https.POST("{\"temperature\":"+String(sensorAverage[0]) + 
                                      ",\"humidity\":"+String(sensorAverage[1]) + 
                                      ",\"hic\":"+String(sensorAverage[2]) + 
                                      ",\"airquality\":"+String(sensorAverage[3]) + "}");
            String payload = https.getString();
            Serial.println(httpCode);
            Serial.println(payload);
            https.end();

            // Reset the preprocessing index
            currentIndex = 0;
        }
        else{
            Serial.println("Error in WiFi connection");
        }
        delay(1000*sendinginterval);
    }
}

// Alert system
void buzzerBeep(int beeps) {
    for (int x = 0; x < beeps; x++) {
        digitalWrite(BUZZER, HIGH);
        delay(500);
        digitalWrite(BUZZER, LOW);
        delay(500);
    }
}


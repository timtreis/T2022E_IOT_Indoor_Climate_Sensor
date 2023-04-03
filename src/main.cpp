#include <DHT.h>
#include <ESP8266WiFi.h>
#include "MQ135.h"

// replace with your channel’s thingspeak API key and your SSID and password
String apiKey = "LQOJHQX7GUOXXZTH";
const char *ssid = "FRITZ!Box 7590 NT 2.4";
const char *password = "25470496396785537215";
const char* server = "api.thingspeak.com";
 
#define DHT_PIN 12
#define DHT_TYPE DHT11 
#define MQ135_PIN A0

unsigned long previousMillis = 0;
unsigned long interval = 30000;

MQ135 mq135_sensor = MQ135(MQ135_PIN);
DHT dht(DHT_PIN, DHT_TYPE);
WiFiClient client;

void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
}

void setup() {
  Serial.begin(115200);
  initWiFi();
  Serial.print("RRSI: ");
  Serial.println(WiFi.RSSI());



}

 
void loop() {

  unsigned long currentMillis = millis();
  // if WiFi is down, try reconnecting every CHECK_WIFI_TIME seconds
  if ((WiFi.status() != WL_CONNECTED) && (currentMillis - previousMillis >= interval)) {

    Serial.print(millis());
    Serial.println("Reconnecting to WiFi...");
    WiFi.disconnect();
    WiFi.begin(ssid, password);
    Serial.println(WiFi.localIP());
    //Alternatively, you can restart your board
    ESP.restart();
    Serial.println(WiFi.RSSI());
    previousMillis = currentMillis;

  } else {

      dht.begin();
      float humidity_values[20];
      // float temperature_values[20];

      for (int i=0; i<20; i++) {
        humidity_values[i] = dht.readHumidity();
        // temperature_values[i] = dht.readTemperature();
        delay(200);
      }

      float h;
      float t;

      for (int i=10; i<20; i++) {
            h = h + humidity_values[i];
            // t = t + temperature_values[i];
      }

      h = h / 10;
      // t = t / 10;
      t = 22.0;

      for (int i=0; i<500; i++) {
         // Preheat MQ135 Sensor
         float ppm = mq135_sensor.getPPM();
         delay(50);
      }

      float rzero_values[20];
      float correctedRZero_values[20];
      float resistance_values[20];
      float ppm_values[20];
      float correctedPPM_values[20];
      float rzero_avg = 0;
      float correctedRZero_avg = 0;
      float resistance_avg = 0;
      float ppm_avg = 0;
      float p;

      for (int i=0; i<20; i++) {
         rzero_values[i] = mq135_sensor.getRZero();
         correctedRZero_values[i] = mq135_sensor.getCorrectedRZero(t, h);
         resistance_values[i] = mq135_sensor.getResistance();
         ppm_values[i] = mq135_sensor.getPPM();
         correctedPPM_values[i] = mq135_sensor.getCorrectedPPM(t, h);
         delay(500);
      }

      for (int i=0; i<20; i++) {
         rzero_avg = rzero_avg + rzero_values[i];
         correctedRZero_avg = correctedRZero_avg + correctedRZero_values[i];
         resistance_avg = resistance_avg + resistance_values[i];
         ppm_avg = ppm_avg + ppm_values[i];
         p = p + correctedPPM_values[i];
      }

      rzero_avg = rzero_avg / 20;
      correctedRZero_avg = correctedRZero_avg / 20;
      resistance_avg = resistance_avg / 20;
      ppm_avg = ppm_avg / 20;
      Serial.println(p);
      p = p / 20;

      if (isnan(h) || isnan(t) || isnan(p)) {
         Serial.println("Failed to read from DHT sensor!");
         return;
      }
      
      if (client.connect(server, 80)) {
         String postStr = apiKey;
         postStr +="&field1=";
         postStr += String(t);
         postStr +="&field2=";
         postStr += String(h);
         postStr += "&field3=";
         postStr += String(p);
         postStr += "&field4=";
         postStr += String(rzero_avg);
         postStr += "&field5=";
         postStr += String(correctedRZero_avg);
         postStr += "&field6=";
         postStr += String(resistance_avg);
         postStr += "&field7=";
         postStr += String(ppm_avg);
         postStr += "\r\n\r\n";
         
         client.print("POST /update HTTP/1.1\n");
         client.print("Host: api.thingspeak.com\n");
         client.print("Connection: close\n");
         client.print("X-THINGSPEAKAPIKEY: "+apiKey+"\n");
         client.print("Content-Type: application/x-www-form-urlencoded\n");
         client.print("Content-Length: ");
         client.print(postStr.length());
         client.print("\n\n");
         client.print(postStr);
         
         Serial.print("Temperature: ");
         Serial.print(t);
         Serial.println(" °C"); 
         Serial.print("Relative Humidity: ");
         Serial.print(h);
         Serial.println(" %");
         Serial.print("Corrected PPM: ");
         Serial.print(p);
         Serial.println(" ppm");
         Serial.print("Sending data to Thingspeak");
      }

      client.stop();
      delay(120000);

  }

} // void loop
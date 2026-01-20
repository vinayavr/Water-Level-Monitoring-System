#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>

// Pin definitions
#define waterSensorPin A0
#define led1Pin D2      // GPIO4
#define led2Pin D5      // GPIO14 (new)
#define led3Pin D6      // GPIO12
#define led4Pin D7      // GPIO13

// Your WiFi credentials
const char* ssid = "Vinaya's Samsung Galaxy M34";
const char* password = "";

// ThingSpeak settings
const char* thingSpeakServer = "api.thingspeak.com";
String thingSpeakApiKey = "4RUIP27D982EYP65";
unsigned long thingSpeakChannelID = 2920754;

// Zapier Webhook URLs
const char* zapierEmailWebhookURL = ""https://hooks.zapier.com/hooks/catch/22539968/2xoxteq/"";
const char* zapierSMSWebhookURL = ""https://hooks.zapier.com/hooks/catch/22539968/2xoxteq/"";

WiFiClient client;

bool highWaterNotificationSent = false;
bool lowWaterNotificationSent = false;
unsigned long lastNotificationTime = 0;
const unsigned long notificationInterval = 60000;

unsigned long lastThingSpeakUpdate = 0;
const unsigned long thingSpeakInterval = 15000;

void setup() {
  pinMode(waterSensorPin, INPUT);
  pinMode(led1Pin, OUTPUT);
  pinMode(led2Pin, OUTPUT);
  pinMode(led3Pin, OUTPUT);
  pinMode(led4Pin, OUTPUT);

  Serial.begin(115200);
  delay(100);

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  int waterLevel = analogRead(waterSensorPin);
 // Serial.print("Water Level Analog Reading: ");
 // Serial.println(waterLevel);

  if (waterLevel < 200) {
    digitalWrite(led1Pin, LOW);
    digitalWrite(led2Pin, LOW);
    digitalWrite(led3Pin, LOW);
    digitalWrite(led4Pin, LOW);

    if (!lowWaterNotificationSent && millis() - lastNotificationTime > notificationInterval) {
      triggerZapierNotification(zapierEmailWebhookURL, waterLevel, "WARNING: Water level is too low - Tank almost empty");
      triggerZapierNotification(zapierSMSWebhookURL, waterLevel, "WARNING: Water level is too low - Tank almost empty");
      lowWaterNotificationSent = true;
      highWaterNotificationSent = false;
      lastNotificationTime = millis();
    }
  } else if (waterLevel >= 200 && waterLevel < 400) {
    digitalWrite(led1Pin, HIGH);
    digitalWrite(led2Pin, LOW);
    digitalWrite(led3Pin, LOW);
    digitalWrite(led4Pin, LOW);
    lowWaterNotificationSent = false;
    highWaterNotificationSent = false;
  } else if (waterLevel >= 400 && waterLevel < 500) {
    digitalWrite(led1Pin, HIGH);
    digitalWrite(led2Pin, HIGH);
    digitalWrite(led3Pin, LOW);
    digitalWrite(led4Pin, LOW);
  } else if (waterLevel >= 500 && waterLevel < 525) {
    digitalWrite(led1Pin, HIGH);
    digitalWrite(led2Pin, HIGH);
    digitalWrite(led3Pin, HIGH);
    digitalWrite(led4Pin, LOW);
  } else if (waterLevel >= 525) {
    digitalWrite(led1Pin, HIGH);
    digitalWrite(led2Pin, HIGH);
    digitalWrite(led3Pin, HIGH);
    digitalWrite(led4Pin, HIGH);

    if (!highWaterNotificationSent && millis() - lastNotificationTime > notificationInterval) {
      triggerZapierNotification(zapierEmailWebhookURL, waterLevel, "ALERT: Water level is very high - Tank almost full");
      triggerZapierNotification(zapierSMSWebhookURL, waterLevel, "ALERT: Water level is very high - Tank almost full");
      highWaterNotificationSent = true;
      lowWaterNotificationSent = false;
      lastNotificationTime = millis();
    }
  }

  if (millis() - lastThingSpeakUpdate > thingSpeakInterval) {
    sendToThingSpeak(waterLevel);
    lastThingSpeakUpdate = millis();
  }

  delay(500);
}

void sendToThingSpeak(int waterLevel) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = "http://" + String(thingSpeakServer) + "/update?api_key=" + thingSpeakApiKey + "&field1=" + String(waterLevel);
    http.begin(client, url);
    int httpCode = http.GET();

    if (httpCode > 0) {
      String payload = http.getString();
      //Serial.println("ThingSpeak Update Successful, response: " + payload);
    } else {
      Serial.println("ThingSpeak Update Failed, error: " + http.errorToString(httpCode));
    }

    http.end();
  } else {
    Serial.println("WiFi not connected. Cannot update ThingSpeak.");
  }
}

void triggerZapierNotification(const char* webhookURL, int waterLevel, const char* message) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(client, webhookURL);
    http.addHeader("Content-Type", "application/json");

    String payload = "{";
    payload += "\"water_level\":" + String(waterLevel) + ",";
    payload += "\"message\":\"" + String(message) + "\",";
    payload += "\"timestamp\":\"" + String(millis()) + "\"";
    payload += "}";

    Serial.println("Payload: " + payload);  
    int httpCode = http.POST(payload);

    Serial.println("HTTP Response code: " + String(httpCode));
    if (httpCode > 0) {
      Serial.println("Response: " + http.getString());
    } else {
      Serial.println("Error: " + http.errorToString(httpCode));
    }
    http.end();
  } else {
    Serial.println("WiFi not connected. Cannot send notification.");
  }
}
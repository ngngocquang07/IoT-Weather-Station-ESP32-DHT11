#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>
#include "esp_wpa2.h" // Library for ESP32 Enterprise Wi-Fi configuration

// --- WIFI CONFIGURATION (HOME) ---
// Set as default: Use these variables for your personal home Wi-Fi
const char* ssid = "YOUR_HOME_SSID";
const char* password = "YOUR_HOME_PASSWORD";

/* // --- WIFI CONFIGURATION (ENTERPRISE/SCHOOL) ---
// If you need to use a school/work network, comment out the home settings above 
// and uncomment/fill in the details below:
const char* ssid = "ENTERPRISE_SSID"; 
#define EAP_IDENTITY "YOUR_IDENTITY"
#define EAP_PASSWORD "YOUR_PASSWORD"
*/

// --- COMPONENT CONFIGURATION ---
#define DHTPIN 4
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

String url = "http://open-meteo.com";
float outTemp = 0.0;

void setup() {
  Serial.begin(115200);
  dht.begin();

  // Initialize OLED display
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    for(;;);
  }
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0, 10);
  display.print("Connecting to WiFi...");
  display.display();
  
  // Standard Wi-Fi connection (Home mode)
  WiFi.disconnect(true);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  /*
  // Enterprise Wi-Fi connection code (uncomment if using EAP)
  // esp_wifi_sta_wpa2_ent_set_identity((uint8_t *)EAP_IDENTITY, strlen(EAP_IDENTITY));
  // esp_wifi_sta_wpa2_ent_set_username((uint8_t *)EAP_IDENTITY, strlen(EAP_IDENTITY));
  // esp_wifi_sta_wpa2_ent_set_password((uint8_t *)EAP_PASSWORD, strlen(EAP_PASSWORD));
  // esp_wifi_sta_wpa2_ent_enable();
  // WiFi.begin(ssid); 
  */
  
  int counter = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    counter++;
    if(counter > 30) { 
      Serial.println("Connection timeout, retrying...");
      break;
    }
  }
  
  display.clearDisplay();
  display.setCursor(0, 10);
  display.print("WiFi Connected!");
  display.display();
  delay(1000);
}

void loop() {
  float inHumi = dht.readHumidity();
  float inTemp = dht.readTemperature();

  // 2. Fetch external weather data via Wi-Fi
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    
    http.begin("https://open-meteo.com");
    
    int httpCode = http.GET();
    
    if (httpCode > 0) {
      String payload = http.getString();
      
      DynamicJsonDocument doc(2048); 
      DeserializationError error = deserializeJson(doc, payload);
      
      if (!error) {
        if (doc.containsKey("current_weather")) {
          outTemp = doc["current_weather"]["temperature"];
        }
      } else {
        Serial.print("JSON decoding error: ");
        Serial.println(error.c_str());
      }
    } else {
      Serial.print("HTTP connection error, code: ");
      Serial.println(httpCode);
    }
    http.end();
  }

  // Display data on OLED
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(15, 0);
  display.print("MINI WEATHER STATION");
  display.drawFastHLine(0, 10, 128, WHITE);

  display.setCursor(0, 16);
  display.print("Indoor:");
  display.setCursor(0, 28);
  display.setTextSize(2);
  if (isnan(inTemp)) {
    display.print("--.-");
  } else {
    display.print(inTemp, 1);
  }
  display.setTextSize(1);
  display.print("C  ");
  display.setTextSize(2);
  if (isnan(inHumi)) {
    display.print("--");
  } else {
    display.print((int)inHumi);
  }
  display.setTextSize(1);
  display.print("%");

  display.setTextSize(1);
  display.setCursor(0, 48);
  display.print("Outdoor (Internet):");
  display.setCursor(0, 56);
  display.print(outTemp, 1);
  display.print(" C");

  display.display();
  delay(10000); 
}
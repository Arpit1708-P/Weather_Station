#define BLYNK_TEMPLATE_ID "**********"  // Enter your own Template ID
#define BLYNK_TEMPLATE_NAME "********"  // Enter your Own Template Name
#define BLYNK_AUTH_TOKEN "***********************" // Enter the token 

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <ArduinoJson.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>
#include <BlynkSimpleEsp8266.h>

// ===== OLED Display Setup =====
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ===== DHT11 Setup =====
#define DHTPIN 0  // GPIO0
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// ===== Wi-Fi Setup =====
const char* ssid = "********";  // Enter the ssid of your wifi
const char* password = "*******"; // Enter the password

// ===== Time Client Setup =====
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 19800, 60000); // IST (UTC +5:30)

// ===== OpenWeatherMap Setup =====
const String apiKey = "**************";    //  Enter your API Key
const String lat = "*****";                // Enter latitude of the location
const String lon = "*****";                // Enter longititude of the location

// ===== Blynk Timer for periodic sensor data =====
BlynkTimer timer;

// ===== Setup =====
void setup() {
  Serial.begin(115200);
  Wire.begin(4, 5); // SDA = GPIO4, SCL = GPIO5
  dht.begin();

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED failed");
    while (true);
  }

  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Connecting Wi-Fi...");
  display.display();

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Wi-Fi Connected!");
  display.display();
  delay(1000);

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, password);  // Blynk after WiFi connect
  timeClient.begin();

  timer.setInterval(5000L, sendSensorData);  // Send sensor data every 5s
}

// ===== Main Loop =====
void loop() {
  Blynk.run();
  timer.run();
  showTime();
  showRoomTemp();
  showCurrentWeather();
  showForecast();
}

// ===== TIME =====
void showTime() {
  timeClient.update();
  time_t rawTime = timeClient.getEpochTime();

  int hour = (timeClient.getHours() % 12 == 0) ? 12 : timeClient.getHours() % 12;
  int minute = timeClient.getMinutes();
  String ampm = (timeClient.getHours() >= 12) ? "PM" : "AM";

  char timeStr[10];
  sprintf(timeStr, "%02d:%02d %s", hour, minute, ampm.c_str());

  struct tm *timeinfo = localtime(&rawTime);
  char dateStr[20] = {0};
  strftime(dateStr, sizeof(dateStr), "%a, %d %b %Y", timeinfo);

  display.clearDisplay();
  display.setTextColor(WHITE);

  // ===== Top Bar =====
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println(dateStr);

  int rssi = WiFi.RSSI();
  display.setCursor(80, 15);  // Changed from (100,0) to avoid collision
  display.print(rssi);
  display.print(" dBm");

  // ===== Time =====
  display.setTextSize(2);
  display.setCursor(0, 32);
  display.println(timeStr);

  display.display();
  delay(4000);
}

// ===== ROOM TEMPERATURE =====
void showRoomTemp() {
  float temp = dht.readTemperature();
  float humidity = dht.readHumidity();

  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 5);
  display.println("Room Data (DHT11)");

  display.setTextSize(2);
  display.setCursor(0, 25);
  display.print(temp);
  display.print(" C ");

  display.setCursor(0, 45);
  display.print(humidity);
  display.print(" %");

  display.display();
  delay(3000);
}

// ===== CURRENT WEATHER =====
void showCurrentWeather() {
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClient client;
    HTTPClient http;
    String currentURL = "http://api.openweathermap.org/data/2.5/weather?lat=" + lat + "&lon=" + lon + "&units=metric&appid=" + apiKey;

    http.begin(client, currentURL);
    int httpCode = http.GET();

    if (httpCode > 0) {
      String payload = http.getString();
      DynamicJsonDocument doc(1024);
      deserializeJson(doc, payload);

      float temp = doc["main"]["temp"];
      const char* condition = doc["weather"][0]["main"];

      display.clearDisplay();
      display.setTextSize(1);
      display.setCursor(0, 5);
      display.println("Current Weather");

      display.setTextSize(2);
      display.setCursor(0, 25);
      display.print(temp);
      display.print(" C");

      display.setCursor(0, 45);
      display.print(condition);
      display.display();
    } else {
      Serial.println("Failed to fetch current weather");
    }

    http.end();
  }
  delay(4000);
}

// ===== FORECAST =====
void showForecast() {
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClient client;
    HTTPClient http;
    String forecastURL = "http://api.openweathermap.org/data/2.5/forecast?lat=" + lat + "&lon=" + lon + "&units=metric&appid=" + apiKey;

    http.begin(client, forecastURL);
    int httpCode = http.GET();

    if (httpCode > 0) {
      String payload = http.getString();
      DynamicJsonDocument doc(8192);
      deserializeJson(doc, payload);

      JsonArray list = doc["list"];

      for (int day = 1; day <= 3; day++) {
        int index = day * 8;

        if (index < list.size()) {
          float temp = list[index]["main"]["temp"];
          const char* weather = list[index]["weather"][0]["main"];
          const char* dt_txt = list[index]["dt_txt"];

          display.clearDisplay();
          display.setTextSize(1);
          display.setCursor(0, 5);
          display.println(dt_txt);

          display.setTextSize(2);
          display.setCursor(0, 25);
          display.print(temp);
          display.print(" C");

          display.setCursor(0, 45);
          display.print(weather);
          display.display();
        } else {
          display.clearDisplay();
          display.setTextSize(1);
          display.setCursor(0, 20);
          display.print("No forecast data");
          display.setCursor(0, 35);
          display.print("for Day ");
          display.print(day);
          display.display();
        }

        delay(4000);
      }
    } else {
      Serial.println("Forecast fetch failed");
    }

    http.end();
  }
}

// ===== Blynk: Send DHT Data to App =====
void sendSensorData() {
  float temp = dht.readTemperature();
  float humidity = dht.readHumidity();

  Blynk.virtualWrite(V0, temp);     // Temperature to V0
  Blynk.virtualWrite(V1, humidity); // Humidity to V1
}

#include <Arduino.h>
#include "ArduinoJson.h"
#include "TFT_eSPI.h"
#include <SPI.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
#include <FS.h>

void init_lcd(void);
void drawBmp(const char *filename, int16_t x, int16_t y);
uint16_t read16(fs::File &f);
uint32_t read32(fs::File &f);

char ssid[] = "Cosmos";        
char pass[] = "voyager2";
const uint8_t fingerprint[20] = {0x70, 0x66, 0x9A, 0x51, 0xE3, 0x68, 0xAA, 0xA0, 0x4A, 0x9A, 0x41, 0x3B, 0x52, 0xA4, 0x95, 0x2D, 0x23, 0xB3, 0x58, 0xD6};

WiFiClient client;
HTTPClient https;
TFT_eSPI tft = TFT_eSPI();
DynamicJsonDocument insight(1024);

void setup() {
  Serial.begin(115200);
  SPIFFS.begin();
  init_lcd();
  tft.setTextWrap(true, true);
  for (int i = 0; i < 61; i+=2)
  {
    drawBmp("/logo.bmp", 0, i - 60);
    delay(1);
  }
  WiFi.begin(ssid,pass);
  while (WiFi.status() != WL_CONNECTED) {
    drawBmp("/wifi_icon.bmp", 100, 150);
    delay(500);
    tft.fillRect(100, 150, 32, 27, TFT_BLACK);
    delay(500);
  }
  std::unique_ptr<BearSSL::WiFiClientSecure>client_secure(new BearSSL::WiFiClientSecure);
  client_secure->setFingerprint(fingerprint);

  drawBmp("/bg1.bmp", 0, 0);

  https.begin(*client_secure, "https://api.mars.spacexcompanion.app/v1/weather/latest");
  https.addHeader("Content-Type", "text/plain");
  https.GET();
  String payload_nasa = https.getString();
  deserializeJson(insight, payload_nasa);

  int current_sol = insight["sol"];
  String season = insight["season"];
  float avg_temp = insight["air"]["temperature"]["average"];
  float min_temp = insight["air"]["temperature"]["minimum"];
  float max_temp = insight["air"]["temperature"]["maximum"];
  float avg_pres = insight["air"]["pressure"]["average"];
  float min_pres = insight["air"]["pressure"]["minimum"];
  float max_pres = insight["air"]["pressure"]["maximum"];
  float avg_wind = insight["wind"]["speed"]["average"];
  float min_wind = insight["wind"]["speed"]["minimum"];
  float max_wind = insight["wind"]["speed"]["maximum"];

  tft.loadFont("calibri22");
  drawBmp("/bg2.bmp", 0, 0);
  tft.setTextColor(TFT_WHITE, TFT_DARKGREY);
  tft.setCursor(45, 80);
  tft.println("SOL: " + (String)current_sol + " (" + season + ")");

  tft.loadFont("calibri18");
  tft.setTextColor(TFT_WHITE, TFT_DARKGREY);
  tft.setCursor(70, 116);
  tft.println("MAX");
  tft.setCursor(70, 132);
  tft.println("AVG");
  tft.setCursor(70, 148);
  tft.println("MIN");

  tft.setCursor(140, 116);
  tft.println((String)max_temp + " C");
  tft.setCursor(140, 132);
  tft.println((String)avg_temp + " C");
  tft.setCursor(140, 148);
  tft.println((String)min_temp + " C");

  tft.setCursor(70, 184);
  tft.println("MAX");
  tft.setCursor(70, 200);
  tft.println("AVG");
  tft.setCursor(70, 216);
  tft.println("MIN");

  tft.setCursor(140, 184);
  tft.println((String)max_pres + " Pa");
  tft.setCursor(140, 200);
  tft.println((String)avg_pres + " Pa");
  tft.setCursor(140, 216);
  tft.println((String)min_pres + " Pa");

  tft.setCursor(70, 252);
  tft.println("MAX");
  tft.setCursor(70, 268);
  tft.println("AVG");
  tft.setCursor(70, 284);
  tft.println("MIN");

  tft.setCursor(140, 252);
  tft.println((String)max_wind + " m/s");
  tft.setCursor(140, 268);
  tft.println((String)avg_wind + " m/s");
  tft.setCursor(140, 284);
  tft.println((String)min_wind + " m/s");
}

void loop() {
  // put your main code here, to run repeatedly:
}

void init_lcd(void)
{
  pinMode(16, OUTPUT);
  digitalWrite(16, LOW);
  tft.init();
  tft.setRotation(0);
  tft.fillScreen(TFT_BLACK);
  tft.setSwapBytes(true);
}

void drawBmp(const char *filename, int16_t x, int16_t y) {

  if ((x >= tft.width()) || (y >= tft.height())) return;

  fs::File bmpFS;

  // Open requested file on SD card
  bmpFS = SPIFFS.open(filename, "r");

  if (!bmpFS)
  {
    Serial.print("File not found");
    return;
  }

  uint32_t seekOffset;
  uint16_t w, h, row, col;
  uint8_t  r, g, b;

  uint32_t startTime = millis();

  if (read16(bmpFS) == 0x4D42)
  {
    read32(bmpFS);
    read32(bmpFS);
    seekOffset = read32(bmpFS);
    read32(bmpFS);
    w = read32(bmpFS);
    h = read32(bmpFS);

    if ((read16(bmpFS) == 1) && (read16(bmpFS) == 24) && (read32(bmpFS) == 0))
    {
      y += h - 1;

      bool oldSwapBytes = tft.getSwapBytes();
      tft.setSwapBytes(true);
      bmpFS.seek(seekOffset);

      uint16_t padding = (4 - ((w * 3) & 3)) & 3;
      uint8_t lineBuffer[w * 3 + padding];

      for (row = 0; row < h; row++) {
        
        bmpFS.read(lineBuffer, sizeof(lineBuffer));
        uint8_t*  bptr = lineBuffer;
        uint16_t* tptr = (uint16_t*)lineBuffer;
        // Convert 24 to 16 bit colours
        for (uint16_t col = 0; col < w; col++)
        {
          b = *bptr++;
          g = *bptr++;
          r = *bptr++;
          *tptr++ = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
        }

        // Push the pixel row to screen, pushImage will crop the line if needed
        // y is decremented as the BMP image is drawn bottom up
        tft.pushImage(x, y--, w, 1, (uint16_t*)lineBuffer);
      }
      tft.setSwapBytes(oldSwapBytes);
      Serial.print("Loaded in "); Serial.print(millis() - startTime);
      Serial.println(" ms");
    }
    else Serial.println("BMP format not recognized.");
  }
  bmpFS.close();
}

uint16_t read16(fs::File &f) {
  uint16_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read(); // MSB
  return result;
}

uint32_t read32(fs::File &f) {
  uint32_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read();
  ((uint8_t *)&result)[2] = f.read();
  ((uint8_t *)&result)[3] = f.read(); // MSB
  return result;
}
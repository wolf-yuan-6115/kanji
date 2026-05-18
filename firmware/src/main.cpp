#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <time.h>
#include <Wire.h>
#include <Adafruit_SHT4x.h>
#include <esp_heap_caps.h>

#include "app_config.h"
#include "vendor/waveshare_4in26g/EPD_4in26g.h"
#include "vendor/waveshare_4in26g/GUI_Paint.h"
#include "vendor/waveshare_4in26g/fonts.h"

namespace {

constexpr int I2C_SDA_PIN = 5;
constexpr int I2C_SCL_PIN = 6;
constexpr uint32_t I2C_FREQ = 100000;

constexpr uint16_t DISPLAY_WIDTH = EPD_4IN26G_WIDTH;
constexpr uint16_t DISPLAY_HEIGHT = EPD_4IN26G_HEIGHT;
constexpr uint16_t LEFT_PANE_WIDTH = (DISPLAY_WIDTH * 2) / 3;
constexpr uint16_t RIGHT_PANE_X = LEFT_PANE_WIDTH;
constexpr uint16_t RIGHT_PANE_WIDTH = DISPLAY_WIDTH - LEFT_PANE_WIDTH;
constexpr uint16_t RIGHT_BOTTOM_ZONE_HEIGHT = 120;
constexpr uint16_t RIGHT_BOTTOM_Y = DISPLAY_HEIGHT - RIGHT_BOTTOM_ZONE_HEIGHT;

constexpr uint32_t CLOCK_UPDATE_MS = 5UL * 60UL * 1000UL;
constexpr uint32_t SENSOR_UPDATE_MS = 5UL * 60UL * 1000UL;
constexpr uint32_t WEATHER_UPDATE_MS = 60UL * 60UL * 1000UL;
constexpr uint32_t NTP_RESYNC_MS = 24UL * 60UL * 60UL * 1000UL;

struct WeatherData {
  bool valid = false;
  int temp_c = 0;
  int humidity = 0;
  int weather_code = 0;
  int wind_kmph = 0;
  char description[40] = "--";
};

struct SensorData {
  bool valid = false;
  float temp_c = 0.0f;
  float humidity = 0.0f;
};

UBYTE* g_framebuffer = nullptr;
UDOUBLE g_framebuffer_size = 0;
bool g_using_psram = false;

Adafruit_SHT4x g_sht4;
bool g_sensor_ok = false;
WeatherData g_weather;
SensorData g_sensor;

uint32_t g_last_clock_update = 0;
uint32_t g_last_sensor_update = 0;
uint32_t g_last_weather_update = 0;
uint32_t g_last_ntp_sync = 0;

tm g_timeinfo = {};
bool g_time_ok = false;

void drawWeatherIcon(uint16_t x, uint16_t y, int code) {
  if (code == 113) {
    Paint_DrawCircle(x + 20, y + 20, 12, EPD_4IN26G_YELLOW, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
    Paint_DrawLine(x + 20, y + 2, x + 20, y + 12, EPD_4IN26G_YELLOW, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    Paint_DrawLine(x + 20, y + 28, x + 20, y + 38, EPD_4IN26G_YELLOW, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    Paint_DrawLine(x + 2, y + 20, x + 12, y + 20, EPD_4IN26G_YELLOW, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    Paint_DrawLine(x + 28, y + 20, x + 38, y + 20, EPD_4IN26G_YELLOW, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    return;
  }

  if (code >= 176 && code <= 200) {
    Paint_DrawRectangle(x + 6, y + 10, x + 34, y + 26, EPD_4IN26G_BLACK, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
    Paint_DrawLine(x + 10, y + 28, x + 14, y + 36, EPD_4IN26G_RED, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    Paint_DrawLine(x + 20, y + 28, x + 24, y + 36, EPD_4IN26G_RED, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    Paint_DrawLine(x + 30, y + 28, x + 34, y + 36, EPD_4IN26G_RED, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    return;
  }

  if ((code >= 80 && code <= 99) || code == 263 || code == 266) {
    Paint_DrawRectangle(x + 6, y + 10, x + 34, y + 26, EPD_4IN26G_BLACK, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
    Paint_DrawLine(x + 10, y + 28, x + 10, y + 36, EPD_4IN26G_BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    Paint_DrawLine(x + 20, y + 28, x + 20, y + 36, EPD_4IN26G_BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    Paint_DrawLine(x + 30, y + 28, x + 30, y + 36, EPD_4IN26G_BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    return;
  }

  if (code == 116 || code == 119 || code == 122) {
    Paint_DrawRectangle(x + 6, y + 12, x + 34, y + 30, EPD_4IN26G_BLACK, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
    Paint_DrawCircle(x + 14, y + 12, 6, EPD_4IN26G_BLACK, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
    Paint_DrawCircle(x + 24, y + 10, 7, EPD_4IN26G_BLACK, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
    return;
  }

  Paint_DrawRectangle(x + 6, y + 10, x + 34, y + 28, EPD_4IN26G_BLACK, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
  Paint_DrawLine(x + 10, y + 32, x + 30, y + 32, EPD_4IN26G_YELLOW, DOT_PIXEL_1X1, LINE_STYLE_DOTTED);
}

void drawLayoutBorders() {
  Paint_DrawLine(LEFT_PANE_WIDTH, 0, LEFT_PANE_WIDTH, DISPLAY_HEIGHT - 1, EPD_4IN26G_BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
  Paint_DrawLine(RIGHT_PANE_X, RIGHT_BOTTOM_Y, DISPLAY_WIDTH - 1, RIGHT_BOTTOM_Y, EPD_4IN26G_BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
}

void drawClockPane() {
  char time_buf[16];
  char date_buf[24];
  if (g_time_ok) {
    strftime(time_buf, sizeof(time_buf), "%H:%M", &g_timeinfo);
    strftime(date_buf, sizeof(date_buf), "%a %Y-%m-%d", &g_timeinfo);
  } else {
    snprintf(time_buf, sizeof(time_buf), "--:--");
    snprintf(date_buf, sizeof(date_buf), "No time sync");
  }

  Paint_DrawString_EN(24, 36, "Clock", &Font20, EPD_4IN26G_BLACK, EPD_4IN26G_WHITE);
  Paint_DrawString_EN(24, 140, time_buf, &Font24, EPD_4IN26G_RED, EPD_4IN26G_WHITE);
  Paint_DrawString_EN(24, 190, date_buf, &Font16, EPD_4IN26G_BLACK, EPD_4IN26G_WHITE);

  if (g_time_ok) {
    char tz_buf[24];
    strftime(tz_buf, sizeof(tz_buf), "%Z", &g_timeinfo);
    Paint_DrawString_EN(24, 222, tz_buf, &Font12, EPD_4IN26G_YELLOW, EPD_4IN26G_WHITE);
  }
}

void drawWeatherPane() {
  Paint_DrawString_EN(RIGHT_PANE_X + 14, 20, "Weather", &Font16, EPD_4IN26G_BLACK, EPD_4IN26G_WHITE);
  drawWeatherIcon(RIGHT_PANE_X + 18, 48, g_weather.weather_code);

  char buf[48];
  if (g_weather.valid) {
    snprintf(buf, sizeof(buf), "%d C", g_weather.temp_c);
    Paint_DrawString_EN(RIGHT_PANE_X + 70, 66, buf, &Font20, EPD_4IN26G_RED, EPD_4IN26G_WHITE);

    snprintf(buf, sizeof(buf), "RH %d%%", g_weather.humidity);
    Paint_DrawString_EN(RIGHT_PANE_X + 14, 110, buf, &Font16, EPD_4IN26G_BLACK, EPD_4IN26G_WHITE);

    snprintf(buf, sizeof(buf), "Wind %dkm", g_weather.wind_kmph);
    Paint_DrawString_EN(RIGHT_PANE_X + 14, 138, buf, &Font12, EPD_4IN26G_YELLOW, EPD_4IN26G_WHITE);

    Paint_DrawString_EN(RIGHT_PANE_X + 14, 162, g_weather.description, &Font12, EPD_4IN26G_BLACK, EPD_4IN26G_WHITE);
  } else {
    Paint_DrawString_EN(RIGHT_PANE_X + 14, 100, "No weather", &Font16, EPD_4IN26G_BLACK, EPD_4IN26G_WHITE);
  }
}

void drawSensorZone() {
  Paint_DrawString_EN(RIGHT_PANE_X + 14, RIGHT_BOTTOM_Y + 12, "Room", &Font16, EPD_4IN26G_BLACK, EPD_4IN26G_WHITE);

  char buf[48];
  if (g_sensor.valid) {
    snprintf(buf, sizeof(buf), "T %.1f C", g_sensor.temp_c);
    Paint_DrawString_EN(RIGHT_PANE_X + 14, RIGHT_BOTTOM_Y + 44, buf, &Font16, EPD_4IN26G_RED, EPD_4IN26G_WHITE);

    snprintf(buf, sizeof(buf), "RH %.1f%%", g_sensor.humidity);
    Paint_DrawString_EN(RIGHT_PANE_X + 14, RIGHT_BOTTOM_Y + 72, buf, &Font16, EPD_4IN26G_BLACK, EPD_4IN26G_WHITE);
  } else {
    Paint_DrawString_EN(RIGHT_PANE_X + 14, RIGHT_BOTTOM_Y + 50, "SHT40 offline", &Font16, EPD_4IN26G_BLACK, EPD_4IN26G_WHITE);
  }
}

void renderAndFlush() {
  Paint_NewImage(g_framebuffer, DISPLAY_WIDTH, DISPLAY_HEIGHT, 0, EPD_4IN26G_WHITE);
  Paint_SetScale(4);
  Paint_SelectImage(g_framebuffer);
  Paint_Clear(EPD_4IN26G_WHITE);

  drawLayoutBorders();
  drawClockPane();
  drawWeatherPane();
  drawSensorZone();

  EPD_4IN26G_Display(g_framebuffer);
}

bool connectWiFi(uint32_t timeout_ms) {
  if (WiFi.status() == WL_CONNECTED) {
    return true;
  }

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  const uint32_t start_ms = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - start_ms) < timeout_ms) {
    delay(250);
  }
  return WiFi.status() == WL_CONNECTED;
}

bool syncTime() {
  configTzTime(TIMEZONE_TZ, NTP_SERVER_PRIMARY, NTP_SERVER_SECONDARY);
  for (int i = 0; i < 20; ++i) {
    if (getLocalTime(&g_timeinfo, 500)) {
      g_time_ok = true;
      g_last_ntp_sync = millis();
      return true;
    }
    delay(200);
  }
  g_time_ok = false;
  return false;
}

bool refreshLocalTime() {
  g_time_ok = getLocalTime(&g_timeinfo, 100);
  return g_time_ok;
}

bool fetchWeather() {
  if (WiFi.status() != WL_CONNECTED) {
    return false;
  }

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  if (!http.begin(client, WEATHER_URL)) {
    return false;
  }

  http.setTimeout(7000);
  const int code = http.GET();
  if (code != HTTP_CODE_OK) {
    http.end();
    return false;
  }

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, http.getStream());
  http.end();
  if (err) {
    return false;
  }

  JsonVariant current = doc["current_condition"][0];
  if (current.isNull()) {
    return false;
  }

  g_weather.temp_c = atoi(current["temp_C"] | "0");
  g_weather.humidity = atoi(current["humidity"] | "0");
  g_weather.weather_code = atoi(current["weatherCode"] | "0");
  g_weather.wind_kmph = atoi(current["windspeedKmph"] | "0");

  const char* desc = current["weatherDesc"][0]["value"] | "--";
  strncpy(g_weather.description, desc, sizeof(g_weather.description) - 1);
  g_weather.description[sizeof(g_weather.description) - 1] = '\0';

  g_weather.valid = true;
  g_last_weather_update = millis();
  return true;
}

bool readSensor() {
  if (!g_sensor_ok) {
    g_sensor.valid = false;
    return false;
  }

  sensors_event_t humidity_event;
  sensors_event_t temp_event;
  if (!g_sht4.getEvent(&humidity_event, &temp_event)) {
    g_sensor.valid = false;
    return false;
  }

  g_sensor.temp_c = temp_event.temperature;
  g_sensor.humidity = humidity_event.relative_humidity;
  g_sensor.valid = true;
  g_last_sensor_update = millis();
  return true;
}

bool allocateFramebuffer() {
  g_framebuffer_size = ((DISPLAY_WIDTH % 4 == 0) ? (DISPLAY_WIDTH / 4) : (DISPLAY_WIDTH / 4 + 1)) * DISPLAY_HEIGHT;

  if (psramFound()) {
    g_framebuffer = static_cast<UBYTE*>(heap_caps_malloc(g_framebuffer_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    if (g_framebuffer != nullptr) {
      g_using_psram = true;
      return true;
    }
  }

  g_framebuffer = static_cast<UBYTE*>(malloc(g_framebuffer_size));
  g_using_psram = false;
  return g_framebuffer != nullptr;
}

void printStartupStatus() {
  Serial.printf("Framebuffer bytes: %lu\n", static_cast<unsigned long>(g_framebuffer_size));
  Serial.printf("Framebuffer memory: %s\n", g_using_psram ? "PSRAM" : "INTERNAL");
  Serial.printf("WiFi connected: %s\n", WiFi.status() == WL_CONNECTED ? "yes" : "no");
  Serial.printf("SHT40 online: %s\n", g_sensor_ok ? "yes" : "no");
  Serial.printf("Time synced: %s\n", g_time_ok ? "yes" : "no");
}

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(500);

  DEV_Module_Init();
  EPD_4IN26G_Init();
  EPD_4IN26G_Clear(EPD_4IN26G_WHITE);

  if (!allocateFramebuffer()) {
    Serial.println("Framebuffer allocation failed");
    while (true) {
      delay(1000);
    }
  }

  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN, I2C_FREQ);
  g_sensor_ok = g_sht4.begin(&Wire);
  if (g_sensor_ok) {
    g_sht4.setPrecision(SHT4X_HIGH_PRECISION);
    g_sht4.setHeater(SHT4X_NO_HEATER);
    readSensor();
  }

  connectWiFi(15000);
  syncTime();
  fetchWeather();
  refreshLocalTime();

  printStartupStatus();
  renderAndFlush();

  const uint32_t now = millis();
  g_last_clock_update = now;
  if (g_last_sensor_update == 0) {
    g_last_sensor_update = now;
  }
  if (g_last_weather_update == 0) {
    g_last_weather_update = now;
  }
}

void loop() {
  const uint32_t now = millis();

  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi(10000);
  }

  if ((now - g_last_ntp_sync) >= NTP_RESYNC_MS) {
    syncTime();
  } else {
    refreshLocalTime();
  }

  bool should_render = false;

  if ((now - g_last_sensor_update) >= SENSOR_UPDATE_MS) {
    readSensor();
    should_render = true;
  }

  if ((now - g_last_weather_update) >= WEATHER_UPDATE_MS) {
    if (fetchWeather()) {
      should_render = true;
    }
  }

  if ((now - g_last_clock_update) >= CLOCK_UPDATE_MS) {
    g_last_clock_update = now;
    should_render = true;
  }

  if (should_render) {
    renderAndFlush();
  }

  delay(500);
}

#include <Arduino.h>
#define LGFX_WT32_SC01
#include <LovyanGFX.hpp>
#include <WiFi.h>
#include <HTTPClient.h>

static LGFX lcd;

const char *wifi_ssid = "【WiFiアクセスポイントのSSID】";
const char *wifi_password = "【WiFiアクセスポイントのパスワード】";

const char *background_url = "https://【立ち上げたサーバのホスト名】/instagram-image";

#define BACKGROUND_BUFFER_SIZE 60000
unsigned long background_buffer_length;
unsigned char background_buffer[BACKGROUND_BUFFER_SIZE];

#define UPDATE_CLOCK_INTERVAL   (10 * 1000UL)
#define FONT_COLOR TFT_WHITE

#define UPDATE_BACKGROUND_INTERVAL  (5 * 60 * 1000UL)

#define CLOCK_TYPE_NONE     0
#define CLOCK_TYPE_TIME     1
#define CLOCK_TYPE_DATETIME 2
unsigned char clock_type = CLOCK_TYPE_DATETIME;

#define CLOCK_ALIGN_CENTER        0
#define CLOCK_ALIGN_TOP_LEFT      1
#define CLOCK_ALIGN_BOTTOM_RIGHT  2
unsigned char clock_align = CLOCK_ALIGN_CENTER;

int last_minutes = -1;
unsigned long last_background = 0;

void wifi_connect(const char *ssid, const char *password);
long doHttpGet(String url, uint8_t *p_buffer, unsigned long *p_len);
float calc_scale(int target, int width);

void setup() {
  Serial.begin(115200);

  lcd.init();
  lcd.setRotation(1);

  Serial.printf("width=%d height=%d\n", lcd.width(), lcd.height());

  lcd.setBrightness(128);
  lcd.setColorDepth(16);
  lcd.setFont(&fonts::Font8);
  lcd.setTextColor(FONT_COLOR);

  wifi_connect(wifi_ssid, wifi_password);
  configTzTime("JST-9", "ntp.nict.jp", "ntp.jst.mfeed.ad.jp");
}

void loop() {
  bool background_updated = false;

  unsigned long now = millis();
  if (last_background == 0 || now - last_background >= UPDATE_BACKGROUND_INTERVAL){
    background_buffer_length = BACKGROUND_BUFFER_SIZE;
    long ret = doHttpGet(background_url, background_buffer, &background_buffer_length);
    if (ret != 0){
      Serial.println("doHttpGet Error");
      delay(1000);
      return;
    }

    last_background = now;
    background_updated = true;
  }

  struct tm timeInfo;
  getLocalTime(&timeInfo);
  if (background_updated || last_minutes != timeInfo.tm_min ){
    lcd.drawJpg(background_buffer, background_buffer_length, 0, 0);

    int width = lcd.width();
    if (clock_align == CLOCK_ALIGN_TOP_LEFT || clock_align == CLOCK_ALIGN_BOTTOM_RIGHT){
      width /= 2;
    }

    char str[11];

    if (clock_type == CLOCK_TYPE_NONE){
      // no drawing
    }else
    if (clock_type == CLOCK_TYPE_TIME){
      sprintf(str, "%02d:%02d", timeInfo.tm_hour, timeInfo.tm_min);
      lcd.setTextSize(1);
      lcd.setTextSize(calc_scale(lcd.textWidth(str), width));

      if (clock_align == CLOCK_ALIGN_TOP_LEFT){
        lcd.setCursor(0, 0);
        lcd.setTextDatum(lgfx::top_left);
      }else
      if (clock_align == CLOCK_ALIGN_BOTTOM_RIGHT ){
        lcd.setCursor(width - lcd.textWidth(str), lcd.height());
        lcd.setTextDatum(lgfx::bottom_left);
      }else{
        lcd.setCursor((width - lcd.textWidth(str)) / 2, lcd.height() / 2);
        lcd.setTextDatum(lgfx::middle_left);
      }
      lcd.printf(str);
    }else
    if (clock_type == CLOCK_TYPE_DATETIME){
      sprintf(str, "%02d:%02d", timeInfo.tm_hour, timeInfo.tm_min);
      lcd.setTextSize(1);
      lcd.setTextSize(calc_scale(lcd.textWidth(str), width));

      int fontHeight_1st = lcd.fontHeight();

      if (clock_align == CLOCK_ALIGN_TOP_LEFT){
        lcd.setCursor(0, 0);
        lcd.setTextDatum(lgfx::top_left);
      }else
      if (clock_align == CLOCK_ALIGN_BOTTOM_RIGHT){
        lcd.setCursor(lcd.width() - lcd.textWidth(str), lcd.height() - lcd.fontHeight()); // ToDo y-axis position is not correct.
        lcd.setTextDatum(lgfx::bottom_left);
      }else{
        lcd.setCursor((lcd.width() - lcd.textWidth(str)) / 2, lcd.height() / 2);
        lcd.setTextDatum(lgfx::bottom_left);
      }
      lcd.printf(str);

      sprintf(str, "%04d.%02d.%02d", timeInfo.tm_year + 1900, timeInfo.tm_mon + 1, timeInfo.tm_mday);

      lcd.setTextSize(1);
      lcd.setTextSize(calc_scale(lcd.textWidth(str), width));

      if (clock_align == CLOCK_ALIGN_TOP_LEFT){
        lcd.setCursor(0, fontHeight_1st);
        lcd.setTextDatum(lgfx::top_left);
      }else
      if (clock_align == CLOCK_ALIGN_BOTTOM_RIGHT){
        lcd.setCursor(lcd.width() - lcd.textWidth(str), lcd.height());
        lcd.setTextDatum(lgfx::bottom_left);
      }else{
        lcd.setCursor((width - lcd.textWidth(str)) / 2, lcd.height() / 2);
        lcd.setTextDatum(lgfx::top_left);
      }
      lcd.printf(str);
    }

    last_minutes = timeInfo.tm_min;
  }

  delay(UPDATE_CLOCK_INTERVAL);
}

float calc_scale(int target, int width){
  if( target > width ){
    int scale1 = ceil((float)target / width);
    return 1.0f / scale1;
  }else{
    return width / target;
  }
}

void wifi_connect(const char *ssid, const char *password)
{
  Serial.println("");
  Serial.print("WiFi Connenting");

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(1000);
  }
  Serial.println("");
  Serial.print("Connected : ");
  Serial.println(WiFi.localIP());
}

long doHttpGet(String url, uint8_t *p_buffer, unsigned long *p_len)
{
  HTTPClient http;

  Serial.print("[HTTP] GET begin...\n");
  // configure traged server and url
  http.begin(url);

  Serial.print("[HTTP] GET...\n");
  // start connection and send HTTP header
  int httpCode = http.GET();
  unsigned long index = 0;

  // httpCode will be negative on error
  if (httpCode > 0)
  {
    // HTTP header has been send and Server response header has been handled
    Serial.printf("[HTTP] GET... code: %d\n", httpCode);

    // file found at server
    if (httpCode == HTTP_CODE_OK)
    {
      // get tcp stream
      WiFiClient *stream = http.getStreamPtr();

      // get lenght of document (is -1 when Server sends no Content-Length header)
      int len = http.getSize();
      Serial.printf("[HTTP] Content-Length=%d\n", len);
      if (len != -1 && len > *p_len)
      {
        Serial.printf("[HTTP] buffer size over\n");
        http.end();
        return -1;
      }

      // read all data from server
      while (http.connected() && (len > 0 || len == -1))
      {
        // get available data size
        size_t size = stream->available();

        if (size > 0)
        {
          // read up to 128 byte
          if ((index + size) > *p_len)
          {
            Serial.printf("[HTTP] buffer size over\n");
            http.end();
            return -1;
          }
          int c = stream->readBytes(&p_buffer[index], size);

          index += c;
          if (len > 0)
          {
            len -= c;
          }
        }
        delay(1);
      }
    }
    else
    {
      http.end();
      return -1;
    }
  }
  else
  {
    http.end();
    Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
    return -1;
  }

  http.end();
  *p_len = index;

  return 0;
}
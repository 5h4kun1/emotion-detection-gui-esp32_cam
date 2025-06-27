#include "esp_camera.h"
#include <WiFi.h>
#include "esp_timer.h"
#include "img_converters.h"
#include "Arduino.h"
#include "fb_gfx.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "esp_http_server.h"
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define CAMERA_MODEL_AI_THINKER
#if defined(CAMERA_MODEL_AI_THINKER)
  #define PWDN_GPIO_NUM     32
  #define RESET_GPIO_NUM    -1
  #define XCLK_GPIO_NUM      0
  #define SIOD_GPIO_NUM     26
  #define SIOC_GPIO_NUM     27
  #define Y9_GPIO_NUM       35
  #define Y8_GPIO_NUM       34
  #define Y7_GPIO_NUM       39
  #define Y6_GPIO_NUM       36
  #define Y5_GPIO_NUM       21
  #define Y4_GPIO_NUM       19
  #define Y3_GPIO_NUM       18
  #define Y2_GPIO_NUM        5
  #define VSYNC_GPIO_NUM    25
  #define HREF_GPIO_NUM     23
  #define PCLK_GPIO_NUM     22
#endif

// Define custom I2C pins for LCD
#define LCD_SDA 14
#define LCD_SCL 15

LiquidCrystal_I2C lcd(0x27, 16, 2);
#define RED_LED    16  // Reassigned from GPIO12 to GPIO16
#define GREEN_LED  12  // Reassigned from GPIO13 to GPIO12
#define BLUE_LED   2   // Unchanged
#define FLASH_PIN  4   // Unchanged

void loadEmoji(const uint8_t p0[], const uint8_t p1[], const uint8_t p2[],
               const uint8_t p3[], const uint8_t p4[], const uint8_t p5[]) {
  lcd.createChar(0, const_cast<uint8_t*>(p0));
  lcd.createChar(1, const_cast<uint8_t*>(p1));
  lcd.createChar(2, const_cast<uint8_t*>(p2));
  lcd.createChar(3, const_cast<uint8_t*>(p3));
  lcd.createChar(4, const_cast<uint8_t*>(p4));
  lcd.createChar(5, const_cast<uint8_t*>(p5));
}

void showEmoji(const char* label) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(label);
  lcd.setCursor(13, 0);
  lcd.write(byte(0));
  lcd.write(byte(1));
  lcd.write(byte(2));
  lcd.setCursor(13, 1);
  lcd.write(byte(3));
  lcd.write(byte(4));
  lcd.write(byte(5));
}

void clearLEDs() {
  digitalWrite(RED_LED, LOW);
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(BLUE_LED, LOW);
}

static esp_err_t stream_handler(httpd_req_t *req) {
  camera_fb_t *fb = NULL;
  esp_err_t res = ESP_OK;
  char *part_buf[64];
  static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=123456789000000000000987654321";
  static const char* _STREAM_BOUNDARY = "\r\n--123456789000000000000987654321\r\n";
  static const char* _STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

  res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
  if (res != ESP_OK) return res;

  while (true) {
    fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Camera capture failed");
      res = ESP_FAIL;
    } else {
      size_t hlen = snprintf((char *)part_buf, 64, _STREAM_PART, fb->len);
      res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
      res |= httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
      res |= httpd_resp_send_chunk(req, (const char *)fb->buf, fb->len);
      esp_camera_fb_return(fb);
    }
    if (res != ESP_OK) break;
  }
  return res;
}

void startCameraServer() {
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.server_port = 81;

  httpd_uri_t stream_uri = {
    .uri       = "/stream",
    .method    = HTTP_GET,
    .handler   = stream_handler,
    .user_ctx  = NULL
  };

  httpd_handle_t stream_httpd = NULL;
  if (httpd_start(&stream_httpd, &config) == ESP_OK) {
    httpd_register_uri_handler(stream_httpd, &stream_uri);
  }
}

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  Serial.begin(115200);

  // Initialize I2C for LCD
  Wire.begin(LCD_SDA, LCD_SCL);
  lcd.begin(16, 2);
  lcd.backlight();
  
  // Verify LCD initialization by attempting to write
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Booting...");
  delay(1000); // Allow time to check if display works
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("LCD Init OK");
  delay(1000); // Visual confirmation of LCD working

  pinMode(RED_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(BLUE_LED, OUTPUT);
  pinMode(FLASH_PIN, OUTPUT);
  digitalWrite(FLASH_PIN, LOW);
  clearLEDs();

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_SVGA;
  config.jpeg_quality = 10;
  config.fb_count = 1;
  config.fb_location = CAMERA_FB_IN_DRAM;

  if (esp_camera_init(&config) != ESP_OK) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Camera failed");
    Serial.println("Camera initialization failed!");
    while (true); // Halt if camera fails
  }

  // SoftAP WiFi
  const char* ssid = "C4M_3Y3Z";
  const char* password = "12345678";
  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("AP IP:");
  lcd.setCursor(0, 1);
  lcd.print(IP);
  Serial.print("Stream: http://");
  Serial.print(IP);
  Serial.println(":81/stream");

  startCameraServer();
}

void loop() {
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    cmd.toLowerCase();

    clearLEDs();

    if (cmd == "flash_on") {
      digitalWrite(FLASH_PIN, HIGH);
      return;
    } else if (cmd == "flash_off") {
      digitalWrite(FLASH_PIN, LOW);
      return;
    }

    if (cmd == "happy") {
      digitalWrite(GREEN_LED, HIGH);
      uint8_t p0[8] = { B00000, B00011, B00100, B01000, B01000, B10001, B10001, B10001 };
      uint8_t p1[8] = { B11111, B00000, B00000, B00000, B00000, B10001, B10001, B10001 };
      uint8_t p2[8] = { B00000, B11000, B00100, B00010, B00010, B10001, B10001, B10001 };
      uint8_t p3[8] = { B10000, B10000, B10001, B01000, B01000, B00100, B00011, B00000 };
      uint8_t p4[8] = { B00000, B00000, B00000, B10000, B01111, B00000, B00000, B11111 };
      uint8_t p5[8] = { B00001, B00001, B01001, B10010, B00010, B00100, B11000, B00000 };
      loadEmoji(p0, p1, p2, p3, p4, p5);
      showEmoji("Happy");
    } else if (cmd == "sad") {
      digitalWrite(BLUE_LED, HIGH);
      uint8_t sad0[8] = { B00000, B00011, B00100, B01000, B01000, B10001, B10001, B10001 };
      uint8_t sad1[8] = { B11111, B00000, B00000, B00000, B00000, B10001, B10001, B10001 };
      uint8_t sad2[8] = { B00000, B11000, B00100, B00010, B00010, B10001, B10001, B10001 };
      uint8_t sad3[8] = { B00000, B00000, B11111, B00000, B00000, B00000, B00000, B11111 };
      uint8_t sad4[8] = { B10000, B10000, B10000, B01001, B01000, B00100, B00011, B00000 };
      uint8_t sad5[8] = { B00001, B00001, B00001, B10010, B00010, B00100, B11000, B00000 };
      loadEmoji(sad0, sad1, sad2, sad3, sad4, sad5);
      showEmoji("Sad");
    } else if (cmd == "neutral") {
      digitalWrite(BLUE_LED, HIGH);
      uint8_t n0[8] = { B00000, B00011, B00100, B01000, B01000, B10000, B10001, B10000 };
      uint8_t n1[8] = { B11111, B00000, B00000, B00000, B00000, B00000, B11001, B00000 };
      uint8_t n2[8] = { B00000, B11000, B00100, B00010, B00010, B00001, B11001, B00001 };
      uint8_t n3[8] = { B10000, B10000, B10000, B01000, B01000, B00100, B00011, B00000 };
      uint8_t n4[8] = { B00000, B00000, B11111, B00000, B00000, B00000, B00000, B11111 };
      uint8_t n5[8] = { B00001, B00001, B00001, B00010, B00010, B00100, B11000, B00000 };
      loadEmoji(n0, n1, n2, n3, n4, n5);
      showEmoji("Neutral");
    } else if (cmd == "angry") {
      digitalWrite(RED_LED, HIGH);
      uint8_t a0[8] = { B00000, B00011, B00100, B01000, B01011, B10001, B10000, B10000 };
      uint8_t a1[8] = { B11111, B00000, B00000, B00000, B00000, B10001, B11011, B11011 };
      uint8_t a2[8] = { B00000, B11000, B00100, B00010, B11010, B10001, B00001, B00001 };
      uint8_t a3[8] = { B10000, B10000, B10000, B01001, B01000, B00100, B00011, B00000 };
      uint8_t a4[8] = { B00000, B00000, B11111, B00000, B00000, B00000, B00000, B11111 };
      uint8_t a5[8] = { B00001, B00001, B00001, B00010, B00010, B00100, B11000, B00000 };
      loadEmoji(a0, a1, a2, a3, a4, a5);
      showEmoji("Angry");
    } else {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Unknown:");
      lcd.setCursor(0, 1);
      lcd.print(cmd);
    }
  }
}

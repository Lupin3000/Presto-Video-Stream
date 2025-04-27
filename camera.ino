#include "esp_camera.h"
#include <WiFi.h>
#include <WebServer.h>
#include <DFRobot_LTR308.h>

// === Camera-Pin-Configuration ===
#define PWDN_GPIO_NUM     -1
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM     5
#define Y9_GPIO_NUM       4
#define Y8_GPIO_NUM       6
#define Y7_GPIO_NUM       7
#define Y6_GPIO_NUM       14
#define Y5_GPIO_NUM       17
#define Y4_GPIO_NUM       21
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM       16
#define VSYNC_GPIO_NUM    1
#define HREF_GPIO_NUM     2
#define PCLK_GPIO_NUM     15
#define SIOD_GPIO_NUM     8
#define SIOC_GPIO_NUM     9

// === WLAN Access Point ===
const char *ap_ssid = "ESP32_CAM_AP";
const char *ap_password = "12345678";

WebServer server(80);

// === GPIOs for LED & IR ===
int led = 3;       // Status-LED
int ir_led = 47;   // IR-LED

// === Light Sensor ===
DFRobot_LTR308 light;

// === Streaming-Task ===
TaskHandle_t streamTaskHandle = NULL;

struct StreamParams {
  WiFiClient client;
};

// === MJPEG-Streaming-Task ===
void streamVideo(void *pvParameters) {
  StreamParams* params = (StreamParams*)pvParameters;
  WiFiClient client = params->client;
  delete params;

  String response = "HTTP/1.1 200 OK\r\n";
  response += "Content-Type: multipart/x-mixed-replace; boundary=frame\r\n\r\n";
  client.print(response);

  while (client.connected()) {
    camera_fb_t * fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Error Camera Frame.");
      continue;
    }

    client.print("--frame\r\n");
    client.print("Content-Type: image/jpeg\r\n\r\n");
    client.write(fb->buf, fb->len);
    client.print("\r\n");

    esp_camera_fb_return(fb);
    delay(80);
  }

  client.stop();
  Serial.println("No Stream-Client.");
  vTaskDelete(NULL);
}

// === Start Camera-Webserver ===
void startCameraServer() {
  server.on("/", HTTP_GET, []() {
    server.send(200, "text/html",
      "<html><body><h2>ESP32 Camera Stream</h2><img src='/stream'></body></html>");
  });

  server.on("/stream", HTTP_GET, []() {
    if (!server.client()) return;

    StreamParams* params = new StreamParams();
    params->client = server.client();

    xTaskCreatePinnedToCore(
      streamVideo,         // Task-Function
      "StreamTask",        // Name
      8192,                // Stack-Size
      params,              // Parameter
      1,                   // Priority
      &streamTaskHandle,   // Handle
      1                    // Core 1 für Streaming
    );
  });

  server.begin();
  Serial.println("MJPEG-Server ready.");
}

// === Setup ===
void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();

  pinMode(led, OUTPUT);
  pinMode(ir_led, OUTPUT);
  digitalWrite(ir_led, LOW);

  // === Camera Setup ===
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
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 12;
  config.fb_count = 1;

  if (psramFound()) {
    config.jpeg_quality = 12;
    config.fb_count = 2;
    config.grab_mode = CAMERA_GRAB_LATEST;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.fb_location = CAMERA_FB_IN_DRAM;
  }

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Error Camera Initialize: 0x%x\n", err);
    return;
  }

  sensor_t *s = esp_camera_sensor_get();
  if (s->id.PID == OV3660_PID) {
    s->set_vflip(s, 1);
    s->set_brightness(s, 1);
    s->set_saturation(s, -2);
  }

  s->set_framesize(s, FRAMESIZE_QVGA); // 320x240

  // === Initialize Light sensor (after Camera!) ===
  while (!light.begin()) {
    Serial.println("Error Sensor Initialize");
    delay(1000);
  }
  Serial.println("Sensor ready.");

  // === Start WLAN-AP ===
  WiFi.softAP(ap_ssid, ap_password);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("Access Point IP: ");
  Serial.println(IP);

  startCameraServer();

  digitalWrite(led, HIGH);
  Serial.println("System ready: http://" + IP.toString());
}

// === Loop ===
void loop() {
  server.handleClient();

  // === Read LUX ===
  uint32_t raw = light.getData();
  float lux = light.getLux(raw);

  Serial.print("LUX: ");
  Serial.println(lux);

  if (lux < 100) {
    digitalWrite(ir_led, HIGH);
  } else {
    digitalWrite(ir_led, LOW);
  }

  delay(1000);
}

#include <WiFi.h>
#include <WebServer.h>
#include <WiFiClientSecure.h>
#include <ESP32Servo.h>
#include <PubSubClient.h>
#include <HTTPClient.h>
#include "DHT.h"
#include "webpage.h"

// WiFi
//const char* ssid     = "Bao Uyen Cafe";        
//const char* password = "68686868";  
// WiFi
//const char* ssid     = "CuongYet";        
//const char* password = "aaaaaaaa";  

// WiFi
//const char* ssid     = "Cuong";        
//const char* password = "vi123456";  

// WiFi
const char* ssid     = "MangDayKTX H1-803";        
const char* password = "h1803h1803";  
// MQTT
const char* mqtt_server   = "c4c3e7f0542c4c4e99afe483bac974e6.s1.eu.hivemq.cloud";
const int   mqtt_port     = 8883;
const char* mqtt_username = "dominhhoi123";
const char* mqtt_password = "#gtCEUS4AnG!qHm";
const char* mqtt_topic_ai_fire = "esp32cam/fire_detection";

//TELEGRAM BOT
#define BOT_TOKEN "8235950088:AAG1UySiu54natGR8l_XKTDGsZH6hUvAwLo" 
#define CHAT_ID "8214051666"      
// Chống spam
bool alertSent = false;
unsigned long lastAlertMillis = 0;
const unsigned long ALERT_COOLDOWN = 30UL * 1000; // 30 giây

// Sensors pins
const int smokePin      = 34;
const int flamePin      = 26;
const int DHTPin        = 27;
const int ALARM_LED_PIN = 14;
const int BUZZER_PIN    = 32;
const int DOOR_SERVO_PIN = 25;

#define DHTTYPE DHT11
DHT dht(DHTPin, DHTTYPE);
Servo doorServo;
WebServer server(80);
WiFiClientSecure espClient;
PubSubClient mqttClient(espClient);

// Sensor data
float temperatureDHT = 0;
float humidity       = 0;
int   smokeBaseline  = 0;
int   smokeCurrent   = 0;
bool  fireAlert      = false;
bool  aiFireDetected = false;

// Door state
enum DoorMode {
  DOOR_AUTO = 0,
  DOOR_FORCE_OPEN = 1,
  DOOR_FORCE_CLOSE = 2
};

DoorMode doorMode = DOOR_AUTO;
bool doorOpen = false;

// LED blink
unsigned long previousBlinkMillis = 0;
bool ledState = false;
const unsigned long blinkInterval = 250;

// MQ2 threshold
const int SMOKE_DELTA = 400;
const int SMOKE_CONFIRM_COUNT = 2;
int smokeHitCount = 0;
int lastSmokeValue = 0;

// telegram send
bool sendTelegramAlert() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[TELEGRAM] No WiFi");
    return false;
  }

  Serial.println("[TELEGRAM] Sending alert...");
  
  String message = "🔥CẢNH BÁO CHÁY!\n";
  message += "Nhiệt độ: *" + String(temperatureDHT, 1) + "°C*\n";
  message += "Độ ẩm: *" + String(humidity, 0) + "%*\n";
  message += "Khí Gas/Khói: *" + String(smokeCurrent) + "* (ADC)\n";
  message += "AI: *" + String(aiFireDetected ? "PHÁT HIỆN" : "KHÔNG") + "*\n";
  
  message.replace(" ", "%20");
  message.replace("\n", "%0A");
  message.replace("*", "%2A");
  
  HTTPClient http;
  WiFiClientSecure client;
  client.setInsecure(); 
  
  String url = "https://api.telegram.org/bot";
  url += BOT_TOKEN;
  url += "/sendMessage?chat_id=";
  url += CHAT_ID;
  url += "&text=";
  url += message;
  url += "&parse_mode=Markdown";
  
  http.begin(client, url);
  http.setTimeout(5000);
  
  int httpCode = http.GET();
  
  if (httpCode == 200) {
    Serial.println("[TELEGRAM] Alert sent OK!");
    http.end();
    return true;
  } else {
    Serial.print("[TELEGRAM] Send failed: ");
    Serial.println(httpCode);
    http.end();
    return false;
  }
}

//WIFI SETUP
void setupWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting WiFi");
  
  int retry = 0;
  while (WiFi.status() != WL_CONNECTED && retry < 40) {
    delay(500);
    Serial.print(".");
    retry++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi OK!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nWiFi FAILED!");
  }
}

//MQTT CALLBACK
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  
  if (String(topic) == mqtt_topic_ai_fire) {
    aiFireDetected = (message == "1" || message == "true" || message == "TRUE");
    Serial.println(aiFireDetected ? "[AI] FIRE!" : "[AI] SAFE");
  }
}

//MQTT RECONNECT
bool reconnectMQTT() {
  static unsigned long lastAttempt = 0;
  
  if (millis() - lastAttempt < 5000) {
    return false;
  }
  lastAttempt = millis();
  
  if (WiFi.status() != WL_CONNECTED) return false;
  
  if (!mqttClient.connected()) {
    Serial.print("MQTT connecting...");
    String clientId = "ESP32-" + String(random(0xffff), HEX);
    
    if (mqttClient.connect(clientId.c_str(), mqtt_username, mqtt_password)) {
      Serial.println(" OK");
      mqttClient.subscribe(mqtt_topic_ai_fire);
      return true;
    } else {
      Serial.print(" FAIL: ");
      Serial.println(mqttClient.state());
      return false;
    }
  }
  return true;
}

void calibrateMQ2() {
  Serial.println("[MQ2] Calibrating baseline...");
  long sum = 0;
  for (int i = 0; i < 20; i++) {
    sum += analogRead(smokePin);
    delay(200);
  }
  smokeBaseline = sum / 20;
  Serial.print("[MQ2] Baseline = ");
  Serial.println(smokeBaseline);
}

//READ SENSORS
void readSensors() {
  smokeCurrent = analogRead(smokePin);
  int flameDigital = digitalRead(flamePin);
  humidity = dht.readHumidity();
  temperatureDHT = dht.readTemperature();

  if (isnan(humidity) || isnan(temperatureDHT)) {
    Serial.println("[DHT] Read ERROR!");
    return;
  }

  bool sensorFire = false;

  if (humidity < 40 && temperatureDHT > 35) {
    sensorFire = true;
    Serial.println("[SENSOR] HIGH TEMP & LOW HUMIDITY!");
  }

  if (flameDigital == LOW) {
    sensorFire = true;
    Serial.println("[FLAME] DETECTED!");
  }

  int smokeDiff = smokeCurrent - lastSmokeValue;

// Nếu khói tăng nhanh bất thường
if (smokeDiff > 80) {
    smokeHitCount++;
    Serial.printf("[SMOKE] RISE %d/%d (Δ=%d)\n",
                  smokeHitCount, SMOKE_CONFIRM_COUNT, smokeDiff);
} 
// Nếu khói đã giảm đủ thấp → reset hoàn toàn
else if (smokeCurrent < smokeBaseline + 200) {
    if (smokeHitCount != 0) {
        Serial.println("[SMOKE] RESET - BACK TO NORMAL");
    }
    smokeHitCount = 0;
}

// Xác nhận khói
if (smokeHitCount >= SMOKE_CONFIRM_COUNT) {
    sensorFire = true;
    Serial.println("[SMOKE] CONFIRMED!");
}

  // Lưu giá trị cho vòng sau
  lastSmokeValue = smokeCurrent;

  fireAlert = sensorFire || aiFireDetected;

  Serial.printf("T:%.1fC H:%.0f%% G:%d AI:%d => %s\n", 
    temperatureDHT, humidity, smokeCurrent, aiFireDetected, 
    fireAlert ? "FIRE!" : "Safe");
}

// DOOR CONTROL
void openDoor() {
  if (!doorOpen) {
    doorServo.write(150);
    doorOpen = true;
    Serial.println("[DOOR] OPENED");
  }
}

void closeDoor() {
  if (doorOpen) {
    doorServo.write(0);
    doorOpen = false;
    Serial.println("[DOOR] CLOSED");
  }
}

//FIRE ALERT HANDLER - LOGIC MỚI ƯU TIÊN BÁO CHÁY
void handleFireAlert() {
  unsigned long now = millis();

  if (fireAlert) {
    // CẢNH BÁO CHÁY - Luôn ưu tiên mở cửa
    digitalWrite(BUZZER_PIN, HIGH);
    
    if (now - previousBlinkMillis >= blinkInterval) {
      previousBlinkMillis = now;
      ledState = !ledState;
      digitalWrite(ALARM_LED_PIN, ledState);
    }
    
    // KHI CÓ CHÁY - BẮT BUỘC MỞ CỬA
    openDoor();
    doorMode = DOOR_AUTO; // Reset về chế độ tự động
    
  } else {
    // KHÔNG CÓ CHÁY - Tắt cảnh báo
    digitalWrite(BUZZER_PIN, LOW);
    digitalWrite(ALARM_LED_PIN, LOW);
    ledState = false;
    
    // Xử lý cửa theo doorMode
    switch (doorMode) {
      case DOOR_AUTO:
        closeDoor();
        break;
        
      case DOOR_FORCE_OPEN:
        openDoor();
        break;
        
      case DOOR_FORCE_CLOSE:
        closeDoor();
        break;
    }
  }
}

//  WEB HANDLERS 
void handleRoot() {
  server.send(200, "text/html", index_html);
}

void handleData() {
  String json = "{";
  json += "\"temperature\":" + String(temperatureDHT, 1) + ",";
  json += "\"humidity\":" + String(humidity, 1) + ",";
  json += "\"gas\":" + String(smokeCurrent) + ",";
  json += "\"fireAlert\":" + String(fireAlert ? "true" : "false") + ",";
  json += "\"aiDetected\":" + String(aiFireDetected ? "true" : "false") + ",";
  json += "\"doorOpen\":" + String(doorOpen ? "true" : "false") + ",";
  json += "\"autoDoor\":" + String(doorMode == DOOR_AUTO ? "true" : "false");
  json += "}";
  
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", json);
}

// XỬ LÝ LỆNH ĐIỀU KHIỂN CỬA TỪ WEB - LOGIC MỚI
void handleDoorControl() {
  if (!server.hasArg("state")) {
    server.send(400, "text/plain", "Missing state");
    return;
  }

  String state = server.arg("state");

  if (state == "open") {
    doorMode = DOOR_FORCE_OPEN;
    openDoor();
    Serial.println("[DOOR] MODE = FORCE_OPEN");
  }
  else if (state == "close") {
    // CHỈ cho phép đóng khi KHÔNG CÓ CHÁY
    if (!fireAlert) {
      doorMode = DOOR_FORCE_CLOSE;
      closeDoor();
      Serial.println("[DOOR] MODE = FORCE_CLOSE");
    } else {
      Serial.println("[DOOR] BLOCK CLOSE - FIRE ALERT ACTIVE");
    }
  }
  else if (state == "auto") {
    doorMode = DOOR_AUTO;
    Serial.println("[DOOR] MODE = AUTO");
  }

  server.send(200, "text/plain", "OK");
}

// SETUP
void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(smokePin, INPUT);
  pinMode(flamePin, INPUT);
  pinMode(ALARM_LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  
  digitalWrite(ALARM_LED_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);

  doorServo.attach(DOOR_SERVO_PIN);
  calibrateMQ2();
  closeDoor();

  dht.begin();
  setupWiFi();

  espClient.setInsecure();
  mqttClient.setServer(mqtt_server, mqtt_port);
  mqttClient.setCallback(mqttCallback);
  reconnectMQTT();

  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.on("/door", handleDoorControl);
  server.begin();

  Serial.println("[SYSTEM] Ready!");
}

// MAIN LOOP
void loop() {
  server.handleClient();

  if (WiFi.status() == WL_CONNECTED) {
    if (!mqttClient.connected()) {
      reconnectMQTT();
    }
    mqttClient.loop();
  }

  // Đọc cảm biến mỗi 2 giây
  static unsigned long lastRead = 0;
  if (millis() - lastRead >= 2000) {
    lastRead = millis();
    readSensors();
  }

  handleFireAlert();

  // Telegram Bot send 
  static unsigned long lastAlertCheck = 0;
  if (millis() - lastAlertCheck >= 3000) {
    lastAlertCheck = millis();
    
    if (fireAlert && !alertSent && (millis() - lastAlertMillis > ALERT_COOLDOWN)) {
      if (sendTelegramAlert()) {
        alertSent = true;
        lastAlertMillis = millis();
      }
    }
    
    if (!fireAlert) {
      alertSent = false;
    }
  }
}
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <ArduinoJson.h>

// Khai báo các thông số
const char* ssid = "Hung"; 
const char* password = "123456789";
const char* mqtt_server = "e5abb49a32fb400ca6e7c5dda3604fd4.s1.eu.hivemq.cloud";
const int mqtt_port = 8883;
const char* mqtt_username = "hung";
const char* mqtt_password = "hung";

// Khai báo chân kết nối
#define DHTPIN D2           // Chân dữ liệu DHT11
#define DHTTYPE DHT11       // Kiểu cảm biến DHT
#define soilMoisturePin A0  // Chân đọc độ ẩm đất và ánh sáng
#define relayFanPin D5      // Chân điều khiển quạt
#define relayPumpPin D6     // Chân điều khiển máy bơm
#define relayLedPin D7      // Chân điều khiển đèn LED
#define soilTransistorPin D3 // Chân điều khiển transistor cho cảm biến độ ẩm đất
#define lightTransistorPin D4 // Chân điều khiển transistor cho cảm biến ánh sáng

// Khai báo các biến
DHT dht(DHTPIN, DHTTYPE);
WiFiClientSecure espClient;
PubSubClient client(espClient);

int thresholdTemperature = 30; // Ngưỡng nhiệt độ
int thresholdHumidity = 70;    // Ngưỡng độ ẩm
int thresholdSoilMoisture = 30; // Ngưỡng độ ẩm đất
int thresholdLightLevel = 30;   // Ngưỡng ánh sáng
bool autoMode = true;           // Chế độ tự động hoặc thủ công
bool fanState = false;          // Trạng thái quạt
bool pumpState = false;         // Trạng thái máy bơm
bool ledState = false;          // Trạng thái đèn LED

int soilMoisture = 0;
int lightLevel = 0;

void setup_wifi() {
  delay(10);
  Serial.print("Kết nối tới WiFi ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  espClient.setInsecure(); // Bỏ qua kiểm tra SSL nếu không có chứng chỉ

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nĐã kết nối tới WiFi");
  Serial.println("Địa chỉ IP: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Đang kết nối tới MQTT...");
    String clientID = "ESP8266Client-";
    clientID += String(random(0xffff), HEX);
    if (client.connect(clientID.c_str(), mqtt_username, mqtt_password)) { 
      Serial.println("Đã kết nối");
      client.subscribe("iot/thresholds");
      client.subscribe("iot/control");
      client.subscribe("iot/mode");
    } else {
      Serial.print("Lỗi, mã lỗi = ");
      Serial.print(client.state());
      delay(2000);
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  DynamicJsonDocument doc(256);
  deserializeJson(doc, payload);

  if (strcmp(topic, "iot/thresholds") == 0) {
    thresholdTemperature = doc["threshold_temperature"];
    thresholdHumidity = doc["threshold_humidity"];
    thresholdSoilMoisture = doc["threshold_soilMoisture"];
    thresholdLightLevel = doc["threshold_lightLevel"];
  }
  
  if (strcmp(topic, "iot/control") == 0) {
    fanState = doc["fan"];
    pumpState = doc["pump"];
    ledState = doc["led"];
    if (!autoMode) {
      digitalWrite(relayFanPin, fanState ? LOW : HIGH);
      digitalWrite(relayPumpPin, pumpState ? HIGH : LOW);
      digitalWrite(relayLedPin, ledState ? LOW : HIGH);
    }
  }

  if (strcmp(topic, "iot/mode") == 0) {
    autoMode = doc["auto"];
  }
}

void setup() {
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  dht.begin();
  pinMode(relayFanPin, OUTPUT);
  pinMode(relayPumpPin, OUTPUT);
  pinMode(relayLedPin, OUTPUT);
  
  pinMode(soilTransistorPin, OUTPUT);
  pinMode(lightTransistorPin, OUTPUT);
  
  digitalWrite(relayFanPin, HIGH);
  digitalWrite(relayPumpPin, LOW);
  digitalWrite(relayLedPin, HIGH);
  
  digitalWrite(soilTransistorPin, LOW); // Tắt transistor ban đầu
  digitalWrite(lightTransistorPin, LOW);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Đọc cảm biến độ ẩm đất
  digitalWrite(soilTransistorPin, HIGH); // Bật transistor cho cảm biến độ ẩm đất
  digitalWrite(lightTransistorPin, LOW);
  delay(100); // Đợi ổn định tín hiệu
  int soilMoistureRaw = analogRead(soilMoisturePin);
  soilMoisture = map(soilMoistureRaw, 0, 1023, 100, 0);
  digitalWrite(soilTransistorPin, LOW); // Tắt transistor sau khi đọc xong

  // Đọc cảm biến ánh sáng
  digitalWrite(lightTransistorPin, HIGH); // Bật transistor cho cảm biến ánh sáng
  delay(100); // Đợi ổn định tín hiệu
  int lightLevelRaw = analogRead(soilMoisturePin);
  lightLevel = map(lightLevelRaw, 0, 1023, 1023, 0);
  digitalWrite(lightTransistorPin, LOW); // Tắt transistor sau khi đọc xong
  
  // Đọc dữ liệu cảm biến DHT11
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  // Hiển thị dữ liệu cảm biến
  Serial.print("Nhiệt độ: ");
  Serial.print(temperature);
  Serial.println(" °C");

  Serial.print("Độ ẩm: ");
  Serial.print(humidity);
  Serial.println(" %");

  Serial.print("Độ ẩm đất: ");
  Serial.print(soilMoisture);
  Serial.println(" %");

  Serial.print("Mức ánh sáng: ");
  Serial.print(lightLevel);
  Serial.println(" lux");

  // Điều khiển các thiết bị dựa trên ngưỡng nếu ở chế độ tự động
  if (autoMode) {
    if (humidity > thresholdHumidity) {
      digitalWrite(relayFanPin, LOW);
      fanState= true;
    } else {
      digitalWrite(relayFanPin, HIGH); 
      fanState= false;
    }

    if (soilMoisture < thresholdSoilMoisture) {
      digitalWrite(relayPumpPin, HIGH);
      pumpState= true;
    } else {
      digitalWrite(relayPumpPin, LOW);
      pumpState= false;
    }
    
    if (lightLevel < thresholdLightLevel) {
      digitalWrite(relayLedPin, LOW);
      ledState = true;
    } else {
      digitalWrite(relayLedPin, HIGH);
      ledState = false;
    }
  } else {
    digitalWrite(relayFanPin, fanState ? LOW : HIGH);
    digitalWrite(relayPumpPin, pumpState ? HIGH : LOW);
    digitalWrite(relayLedPin, ledState ? LOW : HIGH);
  }

  // Gửi dữ liệu cảm biến qua MQTT
  DynamicJsonDocument doc(256);
  doc["temperature"] = temperature;
  doc["humidity"] = humidity;
  doc["soil_moisture"] = soilMoisture;
  doc["light_level"] = lightLevel;
  doc["fan_state"] = fanState; 
  doc["pump_state"] = pumpState; 
  doc["led_state"] = ledState; 
  doc["threshold_temperature"] = thresholdTemperature;
  doc["threshold_humidity"] = thresholdHumidity;
  doc["threshold_soilMoisture"] = thresholdSoilMoisture;
  doc["threshold_lightLevel"] = thresholdLightLevel;

  char mqtt_message[256];
  serializeJson(doc, mqtt_message);
  client.publish("iot/sensors", mqtt_message, true);

  delay(1000); 
}

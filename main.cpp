#include <Arduino.h>
#include <SoftwareSerial.h>
#include <ESP8266WiFi.h>
#include <WebSocketsClient.h>
#include <WiFiClientSecure.h> // Thêm thư viện hỗ trợ kết nối HTTPS

#define RX_PIN 4 // D2
#define TX_PIN 5 // D1
WebSocketsClient webSocket;

SoftwareSerial espSerial(RX_PIN, TX_PIN);

const char* ssid = "SCKT";
const char* password = "huhuhuhu";
const char* host = "nodejs-production-f680.up.railway.app";
const uint16_t port = 443;
//const char* host = "192.168.0.103";
//const uint16_t port = 3000;
const int door = 2;
const int BTN = 0;

WiFiClientSecure client;  // Khởi tạo client để kết nối HTTPS

void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
  switch (type) {
    case WStype_DISCONNECTED:
      Serial.println("[WSc] Disconnected!");
      break;
    case WStype_CONNECTED:
      Serial.printf("[WSc] Connected to: %s\n", host);
      break;
    case WStype_TEXT:
      Serial.printf("[WSc] Received: %s\n", payload);
      if (strcmp((char*)payload, "door_ON") == 0) {
        digitalWrite(door, LOW);
        espSerial.write(1); 
      } else if (strcmp((char*)payload, "door_OFF") == 0) {
        digitalWrite(door, HIGH);
        espSerial.write(0);
      } else if (strcmp((char*)payload, "auto_OFF") == 0) {
        digitalWrite(door, HIGH);
        espSerial.write(3);
      } else if (strcmp((char*)payload, "auto_ON") == 0) {
        digitalWrite(door, HIGH);
        espSerial.write(2);
      break;
      }
    case WStype_BIN:
      Serial.printf("[WSc] Binary received: %u bytes\n", length);
      break;
  }
}

void setup() {
  pinMode(door, OUTPUT);
  pinMode(BTN, INPUT_PULLUP);
  digitalWrite(door, HIGH); // Tắt door lúc đầu
  Serial.begin(115200);
  espSerial.begin(9600);
  Serial.println("ESP8266 Websocket Client");

  // Kết nối Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");

  // Cấu hình kết nối HTTPS
  client.setInsecure(); // Bỏ qua kiểm tra chứng chỉ (an toàn hơn khi dùng trong phát triển)

  // Cấu hình WebSocket để sử dụng client HTTPS
  webSocket.beginSSL(host, port, "/");
  webSocket.onEvent(webSocketEvent);
}

void loop() {
  webSocket.loop();
  static bool isPressed = false;
  
  if (!isPressed && digitalRead(BTN) == LOW) { // Nhấn nút
    isPressed = true;
    webSocket.sendTXT("BTN_PRESSED");
  } else if (isPressed && digitalRead(BTN) == HIGH) { // Nhả nút
    isPressed = false;
    webSocket.sendTXT("BTN_RELEASE");
  }
}

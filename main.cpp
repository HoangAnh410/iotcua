#include <Arduino.h>
#include <SoftwareSerial.h>  // Thư viện giao tiếp Serial qua chân khác
#include <ESP8266WiFi.h>     // Thư viện kết nối Wi-Fi
#include <WebSocketsClient.h> // Thư viện WebSocket client
#include <WiFiClientSecure.h> // Thư viện hỗ trợ kết nối HTTPS

#define RX_PIN 4 // D2: Chân nhận dữ liệu cho Serial
#define TX_PIN 5 // D1: Chân truyền dữ liệu cho Serial
WebSocketsClient webSocket; // Tạo đối tượng WebSocket client

SoftwareSerial espSerial(RX_PIN, TX_PIN); // Tạo đối tượng giao tiếp Serial qua chân RX, TX

// Thông tin Wi-Fi
const char* ssid = "SCKT";         // Tên mạng Wi-Fi
const char* password = "huhuhuhu"; // Mật khẩu Wi-Fi

// Thông tin WebSocket server
const char* host = "nodejs-production-f680.up.railway.app"; // Hostname server
const uint16_t port = 443;                                  // Cổng HTTPS (443)

// Cấu hình chân điều khiển
const int door = 2; // Chân điều khiển cửa
const int BTN = 0;  // Chân đọc trạng thái nút bấm

WiFiClientSecure client;  // Tạo client để kết nối HTTPS an toàn

// Hàm xử lý sự kiện từ WebSocket
void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
  switch (type) {
    case WStype_DISCONNECTED: // Khi ngắt kết nối
      Serial.println("[WSc] Disconnected!");
      break;
    case WStype_CONNECTED: // Khi kết nối thành công
      Serial.printf("[WSc] Connected to: %s\n", host);
      break;
    case WStype_TEXT: // Khi nhận được dữ liệu dạng văn bản từ server
      Serial.printf("[WSc] Received: %s\n", payload);

      // Xử lý dữ liệu nhận được từ server
      if (strcmp((char*)payload, "door_ON") == 0) { // Lệnh mở cửa
        digitalWrite(door, LOW);  // Bật cửa (LOW = hoạt động)
        espSerial.write(1);       // Gửi tín hiệu 1 qua Serial
      } else if (strcmp((char*)payload, "door_OFF") == 0) { // Lệnh đóng cửa
        digitalWrite(door, HIGH); // Tắt cửa (HIGH = không hoạt động)
        espSerial.write(0);       // Gửi tín hiệu 0 qua Serial
      } else if (strcmp((char*)payload, "auto_OFF") == 0) { // Tắt chế độ tự động
        digitalWrite(door, HIGH); // Tắt cửa
        espSerial.write(3);       // Gửi tín hiệu 3 qua Serial
      } else if (strcmp((char*)payload, "auto_ON") == 0) { // Bật chế độ tự động
        digitalWrite(door, HIGH); // Đảm bảo cửa tắt trong chế độ tự động
        espSerial.write(2);       // Gửi tín hiệu 2 qua Serial
      }
      break;

    case WStype_BIN: // Khi nhận dữ liệu dạng nhị phân
      Serial.printf("[WSc] Binary received: %u bytes\n", length);
      break;
  }
}

void setup() {
  pinMode(door, OUTPUT);           // Cấu hình chân điều khiển cửa là đầu ra
  pinMode(BTN, INPUT_PULLUP);      // Cấu hình chân nút bấm là đầu vào với điện trở kéo lên
  digitalWrite(door, HIGH);        // Ban đầu tắt cửa
  Serial.begin(115200);            // Khởi tạo Serial để debug
  espSerial.begin(9600);           // Khởi tạo Serial cho giao tiếp RX/TX
  Serial.println("ESP8266 Websocket Client");

  // Kết nối Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { // Chờ kết nối Wi-Fi
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!"); // Kết nối thành công

  // Cấu hình HTTPS client
  client.setInsecure(); // Bỏ qua kiểm tra chứng chỉ, an toàn hơn trong giai đoạn phát triển

  // Cấu hình WebSocket sử dụng client HTTPS
  webSocket.beginSSL(host, port, "/"); // Kết nối đến server WebSocket
  webSocket.onEvent(webSocketEvent);   // Đăng ký hàm xử lý sự kiện WebSocket
}

void loop() {
  webSocket.loop(); // Xử lý các sự kiện WebSocket

  static bool isPressed = false; // Biến theo dõi trạng thái nút bấm
  
  // Kiểm tra trạng thái nút bấm
  if (!isPressed && digitalRead(BTN) == LOW) { // Khi nút bấm được nhấn
    isPressed = true; // Cập nhật trạng thái
    webSocket.sendTXT("BTN_PRESSED"); // Gửi thông điệp "BTN_PRESSED" đến server
  } else if (isPressed && digitalRead(BTN) == HIGH) { // Khi nút bấm được nhả
    isPressed = false; // Cập nhật trạng thái
    webSocket.sendTXT("BTN_RELEASE"); // Gửi thông điệp "BTN_RELEASE" đến server
  }
}

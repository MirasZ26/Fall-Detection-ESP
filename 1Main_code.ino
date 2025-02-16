#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <base64.h>
#include <time.h>

// OLED Display Configuration
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// MPU6050 Sensor
Adafruit_MPU6050 mpu;

// Wi-Fi Credentials
const char* ssid = "WiFi SSID";
const char* password = "WiFI Password";

// Gmail Credentials
const char* recipient = "Recipient address";
const char* gmail_user = "Sender address";
const char* access_token = "Sender API access token"; // Truncated for security

// Fall Detection Thresholds
const float light_fall_threshold = 1.0;
const float heavy_fall_threshold = 3.0;

// NTP Time Configuration
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 0; // Adjust for timezone
const int daylightOffset_sec = 3600;

void setup() {
    Serial.begin(115200);
    Wire.begin(18, 19);
    
    // Initialize Display
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println("SSD1306 allocation failed");
        while (1);
    }
    display.clearDisplay();
    display.display();

    // Initialize MPU6050
    if (!mpu.begin()) {
        Serial.println("MPU6050 not found!");
        while (1);
    }
    Serial.println("MPU6050 Initialized");
    
    // Connect to WiFi
    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi...");
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.print(".");
    }
    Serial.println("\nWiFi Connected");

    // Sync Time with NTP Server
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    Serial.println("Time Synchronized");
}

void loop() {
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);

    float accel_x = a.acceleration.x;
    float accel_y = a.acceleration.y;
    float accel_z = a.acceleration.z;
    
    float g_total = sqrt(accel_x * accel_x + accel_y * accel_y + accel_z * accel_z);
    String fallType = "none";

    if (g_total > heavy_fall_threshold) {
        fallType = "Heavy";
    } else if (g_total > light_fall_threshold) {
        fallType = "Light";
    }

    // Get current time
    time_t now;
    struct tm timeinfo;
    char timeString[30];
    time(&now);
    localtime_r(&now, &timeinfo);
    strftime(timeString, sizeof(timeString), "%Y-%m-%d %H:%M:%S", &timeinfo);
    
    // Display data on OLED
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.printf("Accel X: %.2f\nAccel Y: %.2f\nAccel Z: %.2f\n", accel_x, accel_y, accel_z);

    if (fallType != "none") {
        display.printf("%s Fall Detected!\nTime: %s\n", fallType.c_str(), timeString);
        sendEmail(recipient, fallType + " Fall Alert", "A " + fallType + " fall was detected at " + String(timeString));
    }
    
    display.display();
    delay(1000);
}

void sendEmail(String to, String subject, String body) {
    Serial.println("Sending email...");
    Serial.printf("To: %s\nSubject: %s\nBody: %s\n", to.c_str(), subject.c_str(), body.c_str());

    HTTPClient http;
    http.begin("https://www.googleapis.com/gmail/v1/users/me/messages/send");
    http.addHeader("Authorization", "Bearer " + String(access_token));
    http.addHeader("Content-Type", "application/json");

    String message = "From: " + String(gmail_user) + "\r\nTo: " + to + "\r\nSubject: " + subject + "\r\n\r\n" + body;
    String encodedMessage = base64::encode(message);
    String jsonPayload = "{\"raw\": \"" + encodedMessage + "\"}";

    int httpResponseCode = http.POST(jsonPayload);
    Serial.println(httpResponseCode == 200 ? "Email Sent Successfully!" : "Email Send Failed!");
    http.end();
}

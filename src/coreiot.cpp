#include "coreiot.h"

// ----------- CONFIGURE THESE! -----------
const char* coreIOT_Server = "app.coreiot.io";  
const char* coreIOT_Token = "mzlqygty8iu5kazvvvaz";   // Device Access Token
const int   mqttPort = 1883;
// ----------------------------------------

WiFiClient espClient;
PubSubClient client(espClient);


void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");

    if (client.connect("ESP32Client", coreIOT_Token, NULL)) {
        
      Serial.println("connected to CoreIOT Server!");
      client.subscribe("v1/devices/me/rpc/request/+");
      Serial.println("Subscribed to v1/devices/me/rpc/request/+");

    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}


void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.println("] ");

  String topicStr = String(topic);
  String requestId = "";
  if (topicStr.startsWith("v1/devices/me/rpc/request/")) {
      requestId = topicStr.substring(26); // Cắt bỏ 26 ký tự đầu để lấy phần ID
  }
  
  // Allocate a temporary buffer for the message
  char message[length + 1];
  memcpy(message, payload, length);
  message[length] = '\0';
  Serial.print("Payload: ");
  Serial.println(message);

  // Parse JSON
  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, message);

  if (error) {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.c_str());
    return;
  }

  const char* method = doc["method"];

  if (strcmp(method, "getStateLED") == 0) {
      Serial.println("Server requested current state.");
      
      // Chuẩn bị topic phản hồi
      String responseTopic = "v1/devices/me/rpc/response/" + requestId;
      
      String jsonValue = led1_state ? "true" : "false";
      String responsePayload = "{\"getStateLED\":" + jsonValue + "}";
      
      client.publish(responseTopic.c_str(), responsePayload.c_str());
      Serial.println("Sent getStateLED response: " + responsePayload);
  }

  else if (strcmp(method, "setStateLED") == 0) {
    // Check params type (could be boolean, int, or string according to your RPC)
    // Example: {"method": "setValueLED", "params": "ON"}
    bool params = doc["params"].as<bool>();
    bool isSuccess = false;

    if (params == true) {
        Serial.println("Device turned ON.");
        led1_state = true;
        // digitalWrite(LED1_PIN, HIGH);
        isSuccess = true;
    } else {   
        Serial.println("Device turned OFF.");
        led1_state = false;
        // digitalWrite(LED1_PIN, LOW);
        isSuccess = true;
    }

    // Bắt buộc phải phản hồi lại để Server ngừng xoay vòng loading trên nút nhấn
    String responseTopic = "v1/devices/me/rpc/response/" + requestId;
    client.publish(responseTopic.c_str(), isSuccess ? "true" : "false");
  } else {
    Serial.print("Unknown method: ");
    Serial.println(method);
  }
}


void setup_coreiot(){

  //Serial.print("Connecting to WiFi...");
  //WiFi.begin(wifi_ssid, wifi_password);
  //while (WiFi.status() != WL_CONNECTED) {
  
  // while (isWifiConnected == false) {
  //   delay(500);
  //   Serial.print(".");
  // }

  while(1){
    if (xSemaphoreTake(xBinarySemaphoreInternet, portMAX_DELAY)) {
      break;
    }
    delay(500);
    Serial.print(".");
  }


  Serial.println(" Connected!");

  client.setServer(coreIOT_Server, mqttPort);
  client.setCallback(callback);

}

void coreiot_task(void *pvParameters){

    setup_coreiot();

    unsigned long lastMsg = 0;

    while(1){

        if (!client.connected()) {
            reconnect();
        }
        client.loop();

        // Sample payload, publish to 'v1/devices/me/telemetry'
        unsigned long now = millis();
        if (now - lastMsg >= 10000) {
            lastMsg = now;
            
            String payload = "{\"temperature\":" + String(glob_temperature) +  ",\"humidity\":" + String(glob_humidity) + "}";
            client.publish("v1/devices/me/telemetry", payload.c_str());
            Serial.println("Published payload: " + payload);
        }

        vTaskDelay(pdMS_TO_TICKS(20));  // Publish every 10 seconds
    }
}
/*
  Rui Santos
  Complete project details at https://RandomNerdTutorials.com/esp-mesh-esp32-esp8266-painlessmesh/
  
  This is a simple example that uses the painlessMesh library: https://github.com/gmag11/painlessMesh/blob/master/examples/basic/basic.ino
*/
#include <ArduinoJson.h>
#include <painlessMesh.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

#define WIFI_SSID "BRYGERVACIO2G"
#define WIFI_PASS "@gervacio123"
#define MESH_PREFIX "PYRO_MESH_NETWORK"
#define MESH_PASSWORD "mypyromeshnetwork"
#define MESH_PORT 5555
#define WIFI_RETRIES 5

const char* serverUrl = "http://raspberrypi:5555/sensor";
Scheduler userScheduler;  // to control your personal task
painlessMesh mesh;

// User stub
void sendMessage();  // Prototype so PlatformIO doesn't complain

Task taskSendMessage(TASK_SECOND * 1, TASK_FOREVER, &sendMessage);

void sendMessage() {
  StaticJsonDocument<200> doc;
  doc["sensor_id"] = mesh.getNodeId();
  doc["sensor_value"] = analogRead(A0);
  // Serialize the JSON object to a string
  String payload;
  serializeJson(doc, payload);

  // Send the JSON payload to the Flask server using HTTP POST
  WiFiClient client;
  HTTPClient http;
  http.begin(client, serverUrl);
  http.addHeader("Content-Type", "application/json");
  int httpResponseCode = http.POST(payload);

  // Check for a successful POST request
  if (httpResponseCode == HTTP_CODE_OK) {
    Serial.println("Sensor data sent to Flask server");
  } else {
    Serial.print("HTTP error code: ");
    Serial.println(httpResponseCode);
  }

  taskSendMessage.setInterval(random(TASK_SECOND * 1, TASK_SECOND * 5));
}

// Needed for painless library
void receivedCallback(uint32_t from, String& msg) {
  Serial.printf("startHere: Received from %u msg=%s\n", from, msg.c_str());
}

void newConnectionCallback(uint32_t nodeId) {
  Serial.printf("--> startHere: New Connection, nodeId = %u\n", nodeId);
}

void wait(unsigned long time) {
    unsigned long start=millis();
    while(millis()-start < time);
}

void startWiFi() {
    int conn_tries = 0;
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    
    WiFi.waitForConnectResult(WIFI_RETRIES * 1000);
    while ((WiFi.status() != WL_CONNECTED) && (conn_tries++ < WIFI_RETRIES)) {
        Serial.println("Connecting to wifi...");
        wait(1000);
    }
    wl_status_t status = WiFi.status();
    switch (status) {
      case WL_IDLE_STATUS:
        Serial.println("[FAILED] The Wi-Fi module is in idle mode");
        break;
      case WL_NO_SSID_AVAIL:
        Serial.println("[FAILED]  No SSID is available to begin a Wi-Fi connection");
        break;
      case WL_CONNECT_FAILED:
        Serial.println("[FAILED] he Wi-Fi connection failed for some reason");
        break;
      case WL_CONNECTION_LOST:
        Serial.println("[FAILED] The Wi-Fi connection is lost");
        break;
      case WL_CONNECTED:
        Serial.println("[SUCCESSS] Connected Successfully");
        break;
      default:
        Serial.println(status);
    }
}
void setup() {
  
  Serial.begin(115200);
  Serial.println("STARTING MESH NODE");


  int numberOfNetworks = WiFi.scanNetworks();

  for (int i = 0; i < numberOfNetworks; i++) {
    Serial.print("Network name: ");
    Serial.println(WiFi.SSID(i));
    Serial.println(WiFi.SSID(i) == WIFI_SSID);
    Serial.print("Signal strength: ");
    Serial.println(WiFi.RSSI(i));
    Serial.println("-----------------------");
  }

  startWiFi();

  //mesh.setDebugMsgTypes( ERROR | MESH_STATUS | CONNECTION | SYNC | COMMUNICATION | GENERAL | MSG_TYPES | REMOTE ); // all types on
  mesh.setDebugMsgTypes(ERROR | STARTUP);  // set before init() so that you can see startup messages

  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  userScheduler.addTask(taskSendMessage);
  taskSendMessage.enable();

  
  

}

void loop() {
  
  mesh.update();
}
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>

#include <SerialTransfer.h>

SerialTransfer myTransfer;

struct __attribute__((packed)) STRUCT {
  float p1a; // probe 1 analog reading
  float p1r; // probe 1 resistance
  float p1t; // probe 1 temperature
  float p2a; // probe 2 analog reading
  float p2r; // probe 2 resistance
  float p2t; // probe 2 temperature
} dataStruct;

// Replace the next variables with your SSID/Password combination
const char* ssid = "<REPLACE WITH YOUR SSID>";
const char* password = "<REPLACE WITH YOUR PASSWORD>";

// Add your MQTT Broker IP address, example:
const char* mqtt_server = "<REPLACE WITH YOUR ADDRESS>";
const char* mqtt_username = "<REPLACE WITH YOUR USERNAME>";
const char* mqtt_password = "<REPLACE WITH YOUR PASSWORD>";

WiFiClient espClient;
PubSubClient client(espClient);

unsigned long lastAlive = 0;

String macAddress = "";
String host = "";

ESP8266WebServer httpServer(80);
ESP8266HTTPUpdateServer httpUpdater;

void setup() {
  Serial.begin(9600);
  Serial.setTimeout(2000);
  myTransfer.begin(Serial);

  // Wait for serial to initialize.
  while(!Serial) { }

  setup_wifi();

  MDNS.begin(host.c_str());

  httpUpdater.setup(&httpServer);
  httpServer.begin();

  MDNS.addService("http", "tcp", 80);
  Serial.printf("HTTPUpdateServer ready! Open http://%s.local/update in your browser\n", host.c_str());

  client.setServer(mqtt_server, 1883);
}

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    WiFi.begin(ssid, password);
    Serial.println("WiFi failed, retrying.");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  Serial.println("MAC address: ");
  macAddress = WiFi.macAddress();
  host = WiFi.macAddress();
  host.replace(":", "-");
  Serial.println(macAddress);
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(macAddress.c_str(), mqtt_username, mqtt_password, String(macAddress + "/alive").c_str(), 0, false, "false")) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }

  httpServer.handleClient();
  MDNS.update();

  client.loop();

  if(myTransfer.available()) {
    myTransfer.rxObj(dataStruct);

    client.publish(String(macAddress + "/probe1Reading").c_str(), String(dataStruct.p1a).c_str());
    client.publish(String(macAddress + "/probe1Resistance").c_str(), String(dataStruct.p1r).c_str());
    client.publish(String(macAddress + "/probe1Temperature").c_str(), String(dataStruct.p1t).c_str());

    client.publish(String(macAddress + "/probe2Reading").c_str(), String(dataStruct.p2a).c_str());
    client.publish(String(macAddress + "/probe2Resistance").c_str(), String(dataStruct.p2r).c_str());
    client.publish(String(macAddress + "/probe2Temperature").c_str(), String(dataStruct.p2t).c_str());
  }

  unsigned long now = millis();

  if (now - lastAlive > 20000) {
    lastAlive = now;
    client.publish(String(macAddress + "/alive").c_str(), "true");
  }
}
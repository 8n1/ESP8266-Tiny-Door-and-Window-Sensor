/*
  Tiny door and window sensor v0.3
  MQTT example
  
  TODO: Add description
  
  
  Resources:
  
  Used MQTT library:
  https://github.com/knolleary/pubsubclient
  
  ESP8266 Arduino core docoumentation:
  https://github.com/esp8266/Arduino/tree/master/doc
  https://github.com/esp8266/Arduino/blob/master/doc/libraries.md  
    
  Espressif FAQ -> Fast wifi connection after power down:
  http://espressif.com/sites/default/files/documentation/espressif_faq_en.pdf
  
  Reading/Writing a long from/to the EEPROM:
  http://playground.arduino.cc/Code/EEPROMReadWriteLong
  
  Converting the wifi signal strength from dbm to %
  http://stackoverflow.com/questions/15797920/how-to-convert-wifi-signal-strength-from-quality-percent-to-rssi-dbm


  MQTT Client states - state()
    -4 : MQTT_CONNECTION_TIMEOUT - the server didn't respond within the keepalive time
    -3 : MQTT_CONNECTION_LOST - the network connection was broken
    -2 : MQTT_CONNECT_FAILED - the network connection failed
    -1 : MQTT_DISCONNECTED - the client is disconnected cleanly
    0 : MQTT_CONNECTED - the cient is connected
    1 : MQTT_CONNECT_BAD_PROTOCOL - the server doesn't support the requested version of MQTT
    2 : MQTT_CONNECT_BAD_CLIENT_ID - the server rejected the client identifier
    3 : MQTT_CONNECT_UNAVAILABLE - the server was unable to accept the connection
    4 : MQTT_CONNECT_BAD_CREDENTIALS - the username/password were rejected
    5 : MQTT_CONNECT_UNAUTHORIZED - the client was not authorized to connect

*/

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <EEPROM.h>

extern "C" {
#include "user_interface.h" // needed for wifi_get_channel()
}

const char* ssid            = "ssid";
const char* password        = "password";

const char* mqtt_server     = "192.168.1.111";
const int mqtt_server_port  = 1883;

const char* battery_topic   = "fhem/sensor4/vbat/set";
const char* state_topic     = "fhem/sensor4/state/set";
const char* rssi_topic      = "fhem/sensor4/rssi/set";
const char* uptime_topic    = "fhem/sensor4/uptime/set";

#define USE_STATIC_IP true  // set to false if a dynamic ip should be used
IPAddress staticIP  (192, 168, 1, 23);
IPAddress gateway   (192, 168, 1, 1);
IPAddress subnet    (255, 255, 255, 0);

#define SWITCH_PIN    4  // GPIO4
#define SHUTDOWN_PIN  5  // GPIO5
#define ACT_LED_PIN   2  // GPIO2 - blue on-board led (active low)

#define R1   4700
#define R2   1000
#define VREF 1.0

#define CONNECT_RETRIES 3

#define EEPROM_SIZE             5   // bytes
#define WIFI_CHANNEL_ADDRESS    0   // 1 byte
#define UPTIME_COUNTER_ADDRESS  1   // 4 bytes

WiFiClient espClient;
PubSubClient client(espClient);

void vreg_shutdown(int blink_count = 0) {

  for (int i = 0; i < blink_count; i++) {
    digitalWrite(ACT_LED_PIN, HIGH);
    delay(200);
    digitalWrite(ACT_LED_PIN, LOW);
    delay(200);
  }

  pinMode(SHUTDOWN_PIN, OUTPUT);
  digitalWrite(SHUTDOWN_PIN, HIGH);
  delay(50);
  
  Serial.printf("\n Time elapsed: %i.%i Seconds\n", int(millis() / 1000), int(millis() % 1000));
  Serial.println(" Shuting down...");

  // store the overall running time (uptime) in the "EEPROM"
  unsigned long uptime_millis = millis();
  EEPROM.begin(EEPROM_SIZE);
  EEPROM.write(UPTIME_COUNTER_ADDRESS, (uptime_millis & 0xFF));
  EEPROM.write(UPTIME_COUNTER_ADDRESS + 1, ((uptime_millis >> 8) & 0xFF));
  EEPROM.write(UPTIME_COUNTER_ADDRESS + 2, ((uptime_millis >> 16) & 0xFF));
  EEPROM.write(UPTIME_COUNTER_ADDRESS + 3, ((uptime_millis >> 24) & 0xFF));
  EEPROM.end();

  digitalWrite(SHUTDOWN_PIN, LOW);
  while (true) {
    delay(5);
  }
}

float get_battery_voltage() {
  int adc_value = analogRead(0);
  float battery_voltage = VREF / 1024.0 * adc_value;
  battery_voltage = battery_voltage * (R1 + R2) / R2;

  return  battery_voltage;
}

void setup()
{
  byte last_wifi_channel, new_wifi_channel;
  
  // For a faster wifi connection after power down - See the Espressif FAQ
  EEPROM.begin(EEPROM_SIZE);
  last_wifi_channel = EEPROM.read(WIFI_CHANNEL_ADDRESS);
  EEPROM.end();
  WRITE_PERI_REG(0x600011f4, 1 << 16 | last_wifi_channel);
  
  WiFi.mode(WIFI_STA);
#if USE_STATIC_IP == true
  WiFi.config(staticIP, gateway, subnet);
#endif
  WiFi.begin(ssid, password);
  
  Serial.begin(115200);
  Serial.println("\n");
  
  // turn on the activity led
  pinMode(ACT_LED_PIN, OUTPUT);
  digitalWrite(ACT_LED_PIN, LOW);
  
  // VBAT
  float vbat = get_battery_voltage();
  char _vbat[String(vbat).length()];
  String(vbat).toCharArray(_vbat, String(vbat).length() + 1);
  
  // RSSI
  int rssi = 2 * (WiFi.RSSI() + 100);   // read RSSI and convert dbm to % 
  char _rssi[String(rssi).length()];
  String(rssi).toCharArray(_rssi, String(rssi).length() + 1);
  
  // STATE
  int switch_state = digitalRead(SWITCH_PIN);
  char _switch_state[1];
  String(switch_state).toCharArray(_switch_state, 2);
  
  // LAST UPTIME
  EEPROM.begin(EEPROM_SIZE);
  long four  = EEPROM.read(UPTIME_COUNTER_ADDRESS);
  long three = EEPROM.read(UPTIME_COUNTER_ADDRESS + 1);
  long two   = EEPROM.read(UPTIME_COUNTER_ADDRESS + 2);
  long one   = EEPROM.read(UPTIME_COUNTER_ADDRESS + 3);
  EEPROM.end();
  unsigned long uptime_millis = ((four << 0) & 0xFF) + ((three << 8) & 0xFFFF) + ((two << 16) & 0xFFFFFF) + ((one << 24) & 0xFFFFFFFF);
  char _uptime[10];
  String(uptime_millis).toCharArray(_uptime, 10);
  
  Serial.println("\nVBAT(V): " + String(_vbat));
  Serial.println("RSSI(%):" + String(_rssi));
  Serial.println("Switch state: " + String(_switch_state));
  Serial.println("Last uptime: " + String(_uptime));
  Serial.println();
  
  // Wait for wifi connection
  Serial.print("Wait for WiFi... ");
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println(" Connection failed.");
    vreg_shutdown(5);
  }
  Serial.printf(" Connected after %i.%is\n",  int(millis() / 1000), int(millis() % 1000));
  Serial.println(" IP: "   + WiFi.localIP().toString() + " (static)");
  
  // store the wifi channel in the EEPROM - See the Espressif FAQ
  new_wifi_channel = wifi_get_channel();
  if (new_wifi_channel != last_wifi_channel) {
    EEPROM.begin(EEPROM_SIZE);
    EEPROM.write(WIFI_CHANNEL_ADDRESS, new_wifi_channel);
    EEPROM.end();
    Serial.println(" Wifi channel " + String(new_wifi_channel) + " saved.");
  }

  // MQTT
  client.setServer(mqtt_server, mqtt_server_port);

  int retry_count = CONNECT_RETRIES;
  while (!client.connected()) {
    Serial.print("\nAttempting MQTT connection...");
    if (client.connect("ESP8266Client")) {
      Serial.println("connected");

      client.publish(battery_topic, _vbat);
      client.publish(rssi_topic, _rssi);
      client.publish(state_topic, _switch_state);
      client.publish(uptime_topic, _uptime);

      vreg_shutdown();
    }
    else {
      Serial.print("failed, rc=");
      Serial.print(client.state()); // States: http://pubsubclient.knolleary.net/api.html#state
      if (retry_count-- <= 0)
        vreg_shutdown(3);

      Serial.println(" Retries left: " + String(retry_count));
      Serial.println(" Trying again in 5 seconds");
      delay(5000);
    }
  }
}

void loop()
{}


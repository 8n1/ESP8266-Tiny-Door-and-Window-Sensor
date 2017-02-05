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

const char* ssid        = "ssid";
const char* password    = "password";

#define USE_STATIC_IP true  // if set to false a dynamic ip address will be used
IPAddress staticIP  (192, 168, 1, 23);
IPAddress gateway   (192, 168, 1, 1);
IPAddress subnet    (255, 255, 255, 0);

const char* mqtt_server     = "192.168.1.111";
const int mqtt_server_port  = 1883;

const char* battery_topic   = "fhem/sensor4/vbat/set";
const char* state_topic     = "fhem/sensor4/state/set";
const char* rssi_topic      = "fhem/sensor4/rssi/set";
const char* uptime_topic    = "fhem/sensor4/uptime/set";

const int switch_pin    = 4;  // GPIO4
const int shutdown_pin  = 5;  // GPIO5
const int act_led_pin   = 2;  // GPIO2 - blue on-board led (active low)

int connection_retry_count = 3;

#define R1   4700
#define R2   1000
#define VREF 1.0

#define EEPROM_SIZE             5   // bytes
#define WIFI_CHANNEL_ADDRESS    0   // 1 byte
#define UPTIME_COUNTER_ADDRESS  1   // 4 bytes

WiFiClient espClient;
PubSubClient client(espClient);

byte last_wifi_channel, new_wifi_channel;
float vbat;
int rssi, switch_state;
unsigned long uptime;
  
float get_battery_voltage() {
  int adc_value = analogRead(0);
  float battery_voltage = VREF / 1024.0 * adc_value;
  battery_voltage = battery_voltage * (R1 + R2) / R2;

  return  battery_voltage;
}

void vreg_shutdown(int error_blink_count = 0) {
  // blink the led if a error occured
  for (int i = 0; i < error_blink_count; i++) {
    digitalWrite(act_led_pin, HIGH);
    delay(500);
    digitalWrite(act_led_pin, LOW);
    delay(500);
  }

  // initiate the shutdown
  pinMode(shutdown_pin, OUTPUT);
  digitalWrite(shutdown_pin, HIGH);
  delay(50);

  Serial.printf("\n Time elapsed: %i.%03i Seconds\n", int(millis() / 1000), int(millis() % 1000));
  Serial.println(" Shuting down ...");

  // store the overall running time (uptime) in the "EEPROM"
  unsigned long uptime_millis = millis();
  EEPROM.begin(EEPROM_SIZE);
  EEPROM.write(UPTIME_COUNTER_ADDRESS,     (uptime_millis)       & 0xFF);
  EEPROM.write(UPTIME_COUNTER_ADDRESS + 1, (uptime_millis >> 8)  & 0xFF);
  EEPROM.write(UPTIME_COUNTER_ADDRESS + 2, (uptime_millis >> 16) & 0xFF);
  EEPROM.write(UPTIME_COUNTER_ADDRESS + 3, (uptime_millis >> 24) & 0xFF);
  EEPROM.end();

  // after setting the pin back to LOW the voltage regulator will be shut down by the ATtiny
  digitalWrite(shutdown_pin, LOW);
  while (true) {
    delay(5);
  }
}

void setup()
{
  // Setup the serial port
  Serial.begin(115200);
  Serial.println("\n");

  // Turn on the activity led
  pinMode(act_led_pin, OUTPUT);
  digitalWrite(act_led_pin, LOW);

  
  // Restore the wifi channel for a faster wifi connection after power down - See the Espressif FAQ.
  // If the wifi channel restored from the EEPROM differs from the one currently used
  // by the AP, it takes longer to connect. This happens for example if the sketch runs for the
  // first time or the AP automatically changes the wifi channel (if activated).
  EEPROM.begin(EEPROM_SIZE);
  last_wifi_channel = EEPROM.read(WIFI_CHANNEL_ADDRESS);
  EEPROM.end();
  WRITE_PERI_REG(0x600011f4, 1 << 16 | last_wifi_channel);

  // Setup the wifi connection
  WiFi.mode(WIFI_STA);
  #if USE_STATIC_IP == true
    WiFi.config(staticIP, gateway, subnet);
  #endif
  WiFi.begin(ssid, password);

  Serial.println("Waiting for WiFi ... ");
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println(" Connection failed.");
    vreg_shutdown(5);
  }
  Serial.printf(" Connected after %i.%03is\n",  int(millis() / 1000), int(millis() % 1000));
  Serial.println(" IP: " + WiFi.localIP().toString());

  // Store the wifi channel in the EEPROM - See the Espressif FAQ
  new_wifi_channel = wifi_get_channel();
  if (new_wifi_channel != last_wifi_channel) {
    EEPROM.begin(EEPROM_SIZE);
    EEPROM.write(WIFI_CHANNEL_ADDRESS, new_wifi_channel);
    EEPROM.end();
    Serial.println(" Wifi channel " + String(new_wifi_channel) + " saved.");
  }


  // VBAT
  vbat = get_battery_voltage();
  Serial.println("\nVBAT (V): " + String(vbat));
  
  // RSSI
  rssi = 2 * (WiFi.RSSI() + 100);   // read RSSI and convert dbm to %
  Serial.println("RSSI (%): " + String(rssi));

  // STATE
  switch_state = digitalRead(switch_pin);
  Serial.println("State (0/1): " + String(switch_state));

  // LAST UPTIME
  EEPROM.begin(EEPROM_SIZE);
  uptime =  ((EEPROM.read(UPTIME_COUNTER_ADDRESS))           & 0xFF);
  uptime += ((EEPROM.read(UPTIME_COUNTER_ADDRESS + 1) << 8)  & 0xFFFF);
  uptime += ((EEPROM.read(UPTIME_COUNTER_ADDRESS + 2) << 16) & 0xFFFFFF);
  uptime += ((EEPROM.read(UPTIME_COUNTER_ADDRESS + 3) << 24) & 0xFFFFFFFF);
  EEPROM.end();
  Serial.println("Last uptime (ms): " + String(uptime));
  
  
  // MQTT
  client.setServer(mqtt_server, mqtt_server_port);

  while (!client.connected()) {
    Serial.print("\nAttempting MQTT connection ... ");
    if (client.connect("ESP8266Client")) {
      Serial.println("Connected.");
      
      Serial.print("Updating topics ... ");
      client.publish(battery_topic, String(vbat).c_str());
      client.publish(rssi_topic,    String(rssi).c_str());
      client.publish(state_topic,   String(switch_state).c_str());
      client.publish(uptime_topic,  String(uptime).c_str());
      Serial.println("Done.");
      
      vreg_shutdown();
    }
    else {
      Serial.print("failed, rc=");
      Serial.print(client.state()); // States: http://pubsubclient.knolleary.net/api.html#state
      if (connection_retry_count-- <= 0) {
        vreg_shutdown(3);
      }

      Serial.println(" Retries left: " + String(connection_retry_count));
      Serial.println(" Trying again in 5 seconds");
      delay(5000);
    }
  }
}

void loop()
{}


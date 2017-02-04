/*
  Tiny door and window sensor v0.3
  IFTTT example

  TODO: Add description
  
  
  Resources:

  IFTTT API:
  https://ifttt.com/maker

  ESP8266 Arduino core docoumentation:
  https://github.com/esp8266/Arduino/tree/master/doc
  https://github.com/esp8266/Arduino/blob/master/doc/libraries.md  
    
  Espressif FAQ -> Fast wifi connection after power down:
  http://espressif.com/sites/default/files/documentation/espressif_faq_en.pdf
  
  Reading/Writing a long from/to the EEPROM:
  http://playground.arduino.cc/Code/EEPROMReadWriteLong

  Converting the wifi signal strength from dbm to %
  http://stackoverflow.com/questions/15797920/how-to-convert-wifi-signal-strength-from-quality-percent-to-rssi-dbm

*/

#include <ESP8266WiFi.h>
#include <EEPROM.h>

extern "C" {
#include "user_interface.h" // needed for wifi_get_channel()
}

const char* ssid            = "ssid";
const char* password        = "password";

const char* ifttt_api_key   = "ifttt_api_key";
// 
const char* ifttt_event_switch_open   = "door_opened_event";
const char* ifttt_event_switch_closed = "door_closed_event";

#define USE_STATIC_IP true
IPAddress staticIP  (192, 168, 1, 23);
IPAddress gateway   (192, 168, 1, 1);
IPAddress subnet    (255, 255, 255, 0);

#define SWITCH_PIN    4  // GPIO4
#define SHUTDOWN_PIN  5  // GPIO5
#define ACT_LED_PIN   2  // GPIO2 - blue on-board led (active low)

#define R1   4700
#define R2   1000
#define VREF 1.0

#define SEND_RETRIES 3

#define EEPROM_SIZE             5   // bytes
#define WIFI_CHANNEL_ADDRESS    0   // 1 byte
#define UPTIME_COUNTER_ADDRESS  1   // 4 bytes

const char* ifttt_event;
String query_string;

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

int send_http_request() {
  const char* host = "maker.ifttt.com";
  String request_url = "/trigger/" + String(ifttt_event) +  "/with/key/" + String(ifttt_api_key);
  request_url = request_url + String(query_string);

  Serial.println("Connecting to " + String(host) + " ...");
  WiFiClient client;
  if (!client.connect(host, 80)) {
    Serial.println("-> Connection failed");
    return false;
  }

  Serial.println("Sending request: " + String(request_url));
  client.print(String("GET " + request_url + " HTTP/1.1\r\n" +
                      "Host: " + String(host) + "\r\n" +
                      "Connection: close\r\n\r\n"));

  unsigned long timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 5000) {
      Serial.println(">>> Client Timeout !");
      client.stop();
      return false;
    }
  }

  while (client.connected()) {
    if (client.available()) {
      String line = client.readStringUntil('\n');
      if (line == "HTTP/1.1 200 OK\r") {
        Serial.println("\n -> REQUEST SUCCESSFULLY SENT. SCENARIO LAUNCHED. <-\n");
        break;
      }
      else {
        Serial.println("\n -> REQUEST FAILED, STATUS CODE NOT 200! <-\n");
        return false;
      }
      Serial.println(line);
    }
    Serial.println("");
  }
  Serial.println("Connection closed.");
  return true;
}

void setup()
{
  byte last_wifi_channel, new_wifi_channel;

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
  query_string = "?value1=" + String(vbat);
  
  // RSSI
  int rssi = 2 * (WiFi.RSSI() + 100);
  query_string += "&value2=" + String(rssi);
  
  // SWITCH STATE
  int switch_state = digitalRead(SWITCH_PIN);
  query_string += "&value3=" + String(switch_state);

  if (switch_state == 0)
    ifttt_event = ifttt_event_switch_closed;
  else
    ifttt_event = ifttt_event_switch_open;
  
  // LAST UPTIME
  EEPROM.begin(EEPROM_SIZE);
  long four  = EEPROM.read(UPTIME_COUNTER_ADDRESS);
  long three = EEPROM.read(UPTIME_COUNTER_ADDRESS + 1);
  long two   = EEPROM.read(UPTIME_COUNTER_ADDRESS + 2);
  long one   = EEPROM.read(UPTIME_COUNTER_ADDRESS + 3);
  EEPROM.end();
  unsigned long uptime_millis = ((four << 0) & 0xFF) + ((three << 8) & 0xFFFF) + ((two << 16) & 0xFFFFFF) + ((one << 24) & 0xFFFFFFFF);
  query_string += "&value4=" + String(uptime_millis);

  Serial.println("\nVBAT(V): " + String(vbat));
  Serial.println("RSSI(%):" + String(rssi));
  Serial.println("Switch state: " + String(switch_state));
  Serial.println("Last uptime(ms): " + String(uptime_millis));
  Serial.println();

  Serial.print("Wait for WiFi... ");
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println(" Connection failed.");
    vreg_shutdown(5);
  }
  Serial.printf(" Connected after %i.%is\n",  int(millis() / 1000), int(millis() % 1000));
  Serial.println(" IP: "   + WiFi.localIP().toString() + " (static)\n");

  new_wifi_channel = wifi_get_channel();
  if (new_wifi_channel != last_wifi_channel) {
    EEPROM.begin(EEPROM_SIZE);
    EEPROM.write(WIFI_CHANNEL_ADDRESS, new_wifi_channel);
    EEPROM.end();
    Serial.println(" Wifi channel " + String(new_wifi_channel) + " saved.");
  }

  // Try to send the HTTP request up to 3 times in case of error
  for (int i = 0; i < SEND_RETRIES; i++) {
    Serial.println("Try: " + String(i + 1));
    int success = send_http_request();
    if (success == true) {
      vreg_shutdown();
    }
    if (i == SEND_RETRIES - 1 && success == false) {
      vreg_shutdown(3);
    }
  }
}

void loop()
{}


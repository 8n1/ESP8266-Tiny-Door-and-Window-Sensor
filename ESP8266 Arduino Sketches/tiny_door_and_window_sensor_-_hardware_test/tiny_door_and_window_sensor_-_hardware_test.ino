/*
 * Tiny door and window sensor v0.3 - Hardware test
 * 
 * Just a simple check to test the hardware.
 * 
*/

#define SWITCH_PIN    4  // GPIO4
#define SHUTDOWN_PIN  5  // GPIO5
#define ACT_LED_PIN   2  // GPIO2 - blue on-board led (active low)

#define R1   4700
#define R2   1000 
#define VREF 1.0

float get_battery_voltage() {
  int adc_value = analogRead(0);
  float battery_voltage = VREF / 1024.0 * adc_value;
  battery_voltage = battery_voltage * (R1 + R2) / R2;
  
  return battery_voltage;
}

void vreg_shutdown() {
  pinMode(SHUTDOWN_PIN, OUTPUT);
  digitalWrite(SHUTDOWN_PIN, HIGH);
  delay(50);  
  digitalWrite(SHUTDOWN_PIN, LOW);
  
  while (true) {
    delay(5);
  }
}

void setup()
{
  Serial.begin(115200);
  Serial.println("\n");
  
  Serial.println("Tiny door and window sensor v0.3 - Hardware test\n");
  
  // turn on the activity led
  pinMode(ACT_LED_PIN, OUTPUT);
  digitalWrite(ACT_LED_PIN, LOW);
  
  // get and print the battery voltage
  float vbat = get_battery_voltage();
  Serial.println("Battery voltage: " + String(vbat) + "V");
  
  // get and print the switch state
  int switch_state = digitalRead(SWITCH_PIN);
  Serial.println("Switch state: " + String(switch_state));
  
  // do something
  delay(5000);
  
  // shut down
  Serial.println("\nSignal the ATtiny to turn off the voltage regulator...");
  vreg_shutdown();
}

void loop()
{}


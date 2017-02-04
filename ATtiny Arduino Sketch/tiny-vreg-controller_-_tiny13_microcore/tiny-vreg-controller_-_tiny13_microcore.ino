/* 
 * 
 * Name:             tiny-vreg-controller.c
 * Version:          0.3
 * Mikrocontroller:  ATtiny13
 * Takt:             1MHz
 * Fuses:            -U lfuse:w:0x6a:m -U hfuse:w:0xff:m (default value)
 * Programmierung:   Johannes S. (joh.raspi - forum-raspberrypi.de)
 * Datum:            22. Dezember 2016
 * 
 * Used Arduino core: https://github.com/MCUdude/MicroCore
 * 
 * Pinout:                                             
 *                   +-------------+                     
 *                 +-+ (PB5)   VCC +-+  VBAT +           
 *                   | (Reset)     |                     
 *                   |             |                     
 *  WAKEUP2        +-+ PB3     PB2 +-+  VREG/LDO Enable (OUTPUT)    
 *  (INPUT)          |                     
 *                   |             |                     
 *  VREG Shutdown  +-+ PB4     PB1 +-+  WAKEUP Switch State (OUTPUT)
 *  (INPUT)          |             |                     
 *                   |             |                     
 *  GND            +-+ GND     PB0 +-+  WAKEUP Switch (INPUT)       
 *                   +-------------+                     
 * 
 */


#include <avr/sleep.h>

//++++++++++++++++++++++++++++++++++++++++
// Pin Configuration
//++++++++++++++++++++++++++++++++++++++++
const int WAKEUP_SWITCH = 0;
const int WAKEUP_SWITCH_STATE = 1;
const int VREG_ENABLE = 2;
const int WAKEUP2 = 3;
const int VREG_SHTDN = 4;

// Anzahl Sekunden nach denen der Spannungsregler automatisch wieder abgeschaltet wird
const unsigned long timerInterval = 60;
//++++++++++++++++++++++++++++++++++++++++

// Arbeitsvariablen
unsigned long start_time;
boolean goto_sleep = true;

//++++++++++++++++++++++++++++++++++++++++
// PinChange ISR - Einstiegspunkt nach dem Tiefschlaf
ISR (PCINT0_vect) {}

//++++++++++++++++++++++++++++++++++++++++
// SETUP FUNCTION
//++++++++++++++++++++++++++++++++++++++++
void setup() 
{
  // GPIOs konfigurieren
  pinMode(VREG_ENABLE, OUTPUT);
  pinMode(VREG_SHTDN, INPUT);
  pinMode(WAKEUP_SWITCH_STATE, OUTPUT);
  pinMode(WAKEUP_SWITCH, INPUT);
  //digitalWrite(WAKEUP_SWITCH, HIGH);  // Pullup Widerstand für den Aufweck Pin
  pinMode(WAKEUP2, INPUT_PULLUP);
  
  // AD Wandler abschalten (benötigt nur unnötig Strom, ~230µA)
  ADCSRA &= ~(1<<ADEN);
  
  // PinChange(PC) Interrupts erlauben
  GIMSK |= (1<<PCIE);
}

//++++++++++++++++++++++++++++++++++++++++
// MAIN LOOP
//++++++++++++++++++++++++++++++++++++++++
void loop() 
{
  //++++++++++++++++++++++++++++++++++++++++
  // Schlafen gehen ?
  //++++++++++++++++++++++++++++++++++++++++
  if (goto_sleep == true)  
  {
    // Spannungsregler abschalten
    digitalWrite(VREG_ENABLE, LOW);
    // Spannungsteiler abschalten
    digitalWrite(WAKEUP_SWITCH_STATE, LOW);

    //++++++++++++++++++++++++++++++++++++++++
    // Auf PC Interrupt an PB0 und PB3 reagieren
    PCMSK |= ((1<<PCINT0) | (1<<PCINT3));

    // The ATtiny13 Microcore uses the watchdog timer interrupt to keep track of the millis.
    // Since the interrupt is still active in the power down sleep mode disable it before
    // entering sleep mode.
    WDTCR &= ~(1<<WDTIE);
    // Schlafmodus(Power-down) konfigurieren
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    // Tiefschlaf aktivieren und auf den PC Interrupt warten
    sleep_mode();
    WDTCR |= (1<<WDTIE);
    
    // Kommt der Interrupt wird die PC ISR ausgeführt und der ATtiny ist wieder wach,
    // anschließend geht es hier wieder weiter
    //sleep_disable();

    // PC Interrupts vorübergehend wieder ignorieren
    PCMSK &= ~((1<<PCINT0) | (1<<PCINT3));
    //++++++++++++++++++++++++++++++++++++++++
    
    // Spannungsregler einschalten
    digitalWrite(VREG_ENABLE, HIGH);
    // (Neue) Startzeit einlesen
    start_time = millis();
    // Genug geschlafen
    goto_sleep = false;
  }
  
  //++++++++++++++++++++++++++++++++++++++++
  // Zustand des Schalters(PB0) auf PB1 ausgeben
  //++++++++++++++++++++++++++++++++++++++++
  digitalWrite(WAKEUP_SWITCH_STATE, digitalRead(WAKEUP_SWITCH));
  
  //++++++++++++++++++++++++++++++++++++++++
  // Verfügbare Zeit abgelaufen ?
  //++++++++++++++++++++++++++++++++++++++++
  if (millis()-start_time >= timerInterval*1000) 
  {
    // Wieder schlafen gehen
    goto_sleep = true;
  }
  
  //++++++++++++++++++++++++++++++++++++++++
  // Ausschalt Signal erkannt ?
  //++++++++++++++++++++++++++++++++++++++++
  if (digitalRead(VREG_SHTDN) == 1) 
  {
    // Womögliches Prellen abfangen
    delay(50);
    // warten bis das HIGH Signal wieder verschwindet
    while (digitalRead(VREG_SHTDN) == 1) {};
    goto_sleep = true;
  }
}

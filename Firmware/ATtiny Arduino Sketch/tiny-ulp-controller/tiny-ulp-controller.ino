/**************************************************************************************************************
+  
+  Name:             tiny-ulp-controller.c
+  Version:          1.0
+  Mikrocontroller:  ATtiny25/45/85
+  Takt:             1MHz
+  Arduino IDE:      1.6.5
+  Programmierung:   Johannes / Frank (joh.raspi/Neueinsteiger - forum-raspberrypi.de)
+  Datum:            7. Oktober 2015
+  
+  Voraussetzung zum kompilieren: ATTiny unterstützung ist wie hier beschrieben installiert:
+  -> Programming an ATtiny w/ Arduino 1.6 (or 1.0): http://highlowtech.org/?p=1695
+  
+  Pinout:                                             
+                  +-------------+                     
+                +-+ (PB5)   VCC +-+  VBAT +           
+                  | (Reset)     |                     
+                  |             |                     
+  TIMER Reset   +-+ PB3     PB2 +-+  VREG Enable (OUTPUT)    
+  (INPUT)         |             |                     
+                  |             |                     
+  VREG Shdn     +-+ PB4     PB1 +-+  WAKEUP Switch State (OUTPUT)
+  (INPUT)         |             |                     
+                  |             |                     
+  GND           +-+ GND     PB0 +-+  WAKEUP Switch (INPUT)       
+                  +-------------+                     
+  
+  Pin Funktionen:
+  PB0 - WAKEUP Switch   - Taster oder Schalter über den der ATTiny aus dem Tiefschlaf geweckt wird
+  PB1 - WAKEUP Switch State - Zustand des Schalters, ermöglicht das abschalten des Spannungsteilers der für die Pegelwandlung benötigt wird
+  PB2 - VREG Enable     - Geht auf den (active high) Enable Eingang des Spannungsreglers
+  PB3 - TIMER Reset     - Ein kurzes HIGH Signal an diesem Pin setzt den Timer Interval zurück
+  PB4 - VREG Shdn       - Ein kurzes HIGH Signal an diesem Pin schaltet den Spannungsreglers ab
+
+  Ablauf:
+  -> Der ATtiny wird durch einen PinChange(PC) Interrupt (Wechsel sowohl von HIGH nach LOW als auch umgekehrt möglich) am Pin PB0 geweckt.
+  -> Der Tiny setht PB2 auf HIGH und aktiviert dadurch den Spannungsregler. 
+  -> Anschließend wird der Zustand des Schalters ausgelesen und wieder über PB1 ausgegeben.
+  -> Dann wird gewartet bis vom ESP ein Signal über PB4 kommt dass den tiny veranlasst den Spannungsregler wieder abzuschalten,
+     oder über PB3 ein Signal kommt dass (dem ESP) nochmal "timerInterval" Sekunden Zeit lässt.
+  -> Passiert beides nicht wird der Spannungsregler um Strom zu sparen automatisch nach "timerInterval" Sekunden wieder abgeschaltet -> PB2 geht auf LOW
+     Zusätzlich wird auch der Pegelwandler abgeschaltet -> PB1 wird auf LOW gesetzt.
+  -> Zu guter letzt geht der ATTiny wieder in den Tiefschlaf(Power-down) und wartet auf den nächsten Interrupt.
+
**************************************************************************************************************/

#include <avr/sleep.h>

//++++++++++++++++++++++++++++++++++++++++
// Pin Konfiguration
//++++++++++++++++++++++++++++++++++++++++
const int WAKEUP_SWITCH = 0;
const int WAKEUP_SWITCH_STATE = 1;
const int VREG_ENABLE = 2;
const int TIMER_RESET = 3;
const int VREG_SHTDN = 4;

// Zeit in Sekunden bevor der Spannungsregler automatisch abgeschaltet wird
const unsigned long timerInterval = 60;
//++++++++++++++++++++++++++++++++++++++++

// Arbeitsvariablen
unsigned long start_time;
boolean allow_sleep = true;

//++++++++++++++++++++++++++++++++++++++++
// PinChange Interrupt Routine - Einstiegspunkt nach dem Tiefschlaf
ISR (PCINT0_vect) {}

//++++++++++++++++++++++++++++++++++++++++
void setup() 
{
  // GPIOs konfigurieren
  pinMode(VREG_ENABLE, OUTPUT);
  pinMode(WAKEUP_SWITCH_STATE, OUTPUT);
  pinMode(TIMER_RESET, INPUT);
  pinMode(VREG_SHTDN, INPUT);
  pinMode(WAKEUP_SWITCH, INPUT);
  //digitalWrite(WAKEUP_SWITCH, HIGH);
  
  // AD Wandler abschalten (benötigt nur unnötig Strom)
  ADCSRA &= ~(1<<ADEN);
  
  // Schlafmodus(Power-down) einstellen
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  
  // PinChange(PC) Interrupts erlauben
  GIMSK |= (1<<PCIE);
}

void loop() 
{
  //++++++++++++++++++++++++++++++++++++++++
  // Schlafen gehen
  //++++++++++++++++++++++++++++++++++++++++
  if (allow_sleep == true)  
  {
    // Spannungsregler abschalten
    digitalWrite(VREG_ENABLE, LOW);
    // Spannungsteiler abschalten
    digitalWrite(WAKEUP_SWITCH_STATE, LOW);

    //++++++++++++++++++++++++++++++++++++++++
    // (Nur) auf PC Interrupt an PB0 reagieren
    PCMSK |= (1<<PCINT0);
    
    // Schlafmodus aktivieren und auf Interrupt warten
    sleep_mode();
    
    // Kommt der Interrupt wird die PC ISR ausgeführt und der ATtiny ist wieder wach,
    // anschließend geht es hier wieder weiter.
    
    // PC Interrupt ignorieren
    PCMSK &= ~(1<<PCINT0);
    //++++++++++++++++++++++++++++++++++++++++
    
    // Spannungsregler einschalten
    digitalWrite(VREG_ENABLE, HIGH);
    // Startzeit einlesen
    start_time = millis();
    // Schlafen gehen verbieten
    allow_sleep = false;
  }
  
  //++++++++++++++++++++++++++++++++++++++++
  // Zustand des Schalter(PB0) über PB1 ausgeben
  //++++++++++++++++++++++++++++++++++++++++
  digitalWrite(WAKEUP_SWITCH_STATE, digitalRead(WAKEUP_SWITCH));
    
  //++++++++++++++++++++++++++++++++++++++++
  // Maximal verfügbare Zeit abgelaufen
  //++++++++++++++++++++++++++++++++++++++++
  if (millis()-start_time >= timerInterval*1000) 
  {
    // Schlafen gehen erlauben
    allow_sleep = true;
  }
  
  //++++++++++++++++++++++++++++++++++++++++
  // Ausschalt Signal erkannt
  //++++++++++++++++++++++++++++++++++++++++
  if (digitalRead(VREG_SHTDN) == 1) 
  {
    // Womögliches Prellen abfangen
    delay(250);
    // warten bis das HIGH Signal wieder verschwindet
    while (digitalRead(VREG_SHTDN) == 1) {};
    allow_sleep = true;
  }

  //++++++++++++++++++++++++++++++++++++++++
  // Reset Signal erkannt
  //++++++++++++++++++++++++++++++++++++++++
  if (digitalRead(TIMER_RESET) == 1) 
  {
    delay(250);
    while (digitalRead(TIMER_RESET) == 1) {};
    // Neue Startzeit setzen
    start_time = millis();
  }
}


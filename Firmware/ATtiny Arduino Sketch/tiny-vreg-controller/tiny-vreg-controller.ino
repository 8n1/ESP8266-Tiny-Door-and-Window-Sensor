/**************************************************************************************************************
+  
+  Name:             tiny-vreg-controller.c
+  Version:          0.2
+  Mikrocontroller:  ATtiny25V/45V/85V
+  Takt:             1MHz
+  Fuses:            -U lfuse:w:0x62:m -U hfuse:w:0xdf:m -U efuse:w:0xff:m  (default value)
+  Programmierung:   Johannes S. (joh.raspi - forum-raspberrypi.de)
+  Datum:            10. Oktober 2015
+  Changelog:
+  * TIMER Reset ersetzt durch zweiten Aufweck-Pin
+
+  Getestet mit der Arduino IDE v1.6.5
+  Voraussetzung zum kompilieren: ATtiny unterstützung ist installiert.
+  (Am besten über den neuen Boards Manager machen: Tools->Board:...->Boards Manager->"attiny")
+  
+  Pinout:                                             
+                   +-------------+                     
+                 +-+ (PB5)   VCC +-+  VBAT +           
+                   | (Reset)     |                     
+                   |             |                     
+  WAKEUP2        +-+ PB3     PB2 +-+  VREG/LDO Enable (OUTPUT)    
+  (INPUT)          |                     
+                   |             |                     
+  VREG Shutdown  +-+ PB4     PB1 +-+  WAKEUP Switch State (OUTPUT)
+  (INPUT)          |             |                     
+                   |             |                     
+  GND            +-+ GND     PB0 +-+  WAKEUP Switch (INPUT)       
+                   +-------------+                     
+  
+  Pin Funktionen:
+  PB0 - WAKEUP Switch       - Taster oder Schalter über den der ATtiny aus dem Tiefschlaf geweckt wird
+  PB1 - WAKEUP Switch State - Der Zustand des Schalters wird auch auf diesem Pin ausgegeben
+  PB2 - VREG Enable         - Muss mit dem (active high) Enable/Shutdown Pin des Spannungsreglers verbunden werden
+  PB3 - WAKEUP2             - Zweiter Aufweck-Pin. Kann genutzt werden um den ATtiny bspw. über einen RTC Alarm aufzuwecken
+  PB4 - VREG Shutdown       - Ein kurzes HIGH Signal an diesem Pin schaltet den Spannungsreglers ab
+
+  Ablauf:
+  1. -> Der ATtiny wird durch einen PinChange(PC) Interrupt (Wechsel sowohl von HIGH nach LOW als auch umgekehrt möglich) am Pin PB0 oder PB3 geweckt.
+  2. -> Der Tiny setzt PB2 auf HIGH und aktiviert dadurch den Spannungsregler (und somit den ESP).
+  3. -> Jetzt wird gewartet bis (vom ESP) ein Signal über PB4 kommt dass ihn veranlasst den Regler wieder abzuschalten,
         Ausserdem wird der Zustand des Schalters(PB0) immer wieder neu eingelesen und über PB1 ausgegeben.
+  4. -> Ist nach "timerInterval" Sekunden noch kein Signal gekommen wird der Spannungsregler um Strom zu sparen vom tiny wieder abgeschaltet -> PB2 geht auf LOW
+        Zusätzlich wird auch PB1 auf LOW gesetzt (deaktiviert den Spannungsteiler/Pegelwandler)
+  5. -> Zu guter letzt geht der ATtiny wieder in den Tiefschlaf(Power-down) und wartet auf den nächsten Interrupt. 
+        Der Stromverbrauch des ATtiny liegt während er schläft bei ca. ~300nA @3.8V. Im Datenblatt steht genauers.
+
**************************************************************************************************************/

#include <avr/sleep.h>

//++++++++++++++++++++++++++++++++++++++++
// Pin Konfiguration
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
  pinMode(WAKEUP2, INPUT);
  digitalWrite(WAKEUP2, HIGH);
  
  // AD Wandler abschalten (benötigt nur unnötig Strom, ~230µA)
  ADCSRA &= ~(1<<ADEN);
  
  // Schlafmodus(Power-down) konfigurieren
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  
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
    
    // Schlafmodus aktivieren
    sleep_enable();
    // Tiefschlaf aktivieren und auf den PC Interrupt warten
    sleep_mode();
    
    // Kommt der Interrupt wird die PC ISR ausgeführt und der ATtiny ist wieder wach,
    // anschließend geht es hier wieder weiter
    sleep_disable();

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
    delay(100);
    // warten bis das HIGH Signal wieder verschwindet
    while (digitalRead(VREG_SHTDN) == 1) {};
    goto_sleep = true;
  }
}


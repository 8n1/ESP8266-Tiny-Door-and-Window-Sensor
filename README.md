# ESP8266-NodeMCU_Tiny-Door-and-Window-Sensor-with-Ultra-Low-Standby-Power

## Intro
Eine für Schalter angepasste Version des Briefkastenwächters. In der config.lua werden 2 Pushingbox Szenarios hinterlegt die direkt nach dem Start abhängig vom Zustand eines GPIO Pins aktiviert werden. Liegt z.B. am Pin ein LOW Pegel an wird das erste Szenario verwendet, sonst das zweite.
...

## Neue Einstellungen (config.lua)
* **CLOSING_DEVID = "xxxxx"**   -> Pushingbox Szernario wenn Zustand von SWITCH_PIN = 0
* **OPENING_DEVID = "xxxxx"**   -> Szenario wenn Zustand von SWITCH_PIN = 1

* **SWITCH_PIN = 6**  -> Pin für den Schalter (kein Pullup/Pulldown aktiviert)

* **vreg_shutdown_pin = 2**   -> Pin der dem ATtiny signalisiert den Spannungsregler abzuschalten (Pin wird wenn alles erledigt ist auf LOW gesetzt)

* **sensor_vcc_pin = 7**  -> Pin über den (kleine) Sensoren mit Storm versorgt werden können (maximal 12mA!) (Pin wird nach dem Start auf HIGH gesetzt)
  
* **SSID = "RaspiAP"**  -> SSID vom Router. Wird für das ermitteln der Signalstärke benötigt

##### Einstellungen für die Fehlerbehebung (Maximale Laufzeit mit der standard konfiguration(3,10): ~50 seconds)
* **max_retries = 3**     -> Wie oft versucht wird den Pushingbox Request zu senden
* **time_between_requests = 10**    -> Wieviele Sekunden zwischen den Versuchen gewartet werden soll
  
* **clear_logs_pin = 5**    -> Ist dieser Pin beim Start mit GND verbunden werden alle Logfiles gelöscht





--------------------------------------
-- NodeMCU I/O Index
--------------------------------------
-- I/O    GPIO     I/O   ESP8266 pin
-- 0 [*]  GPIO16   7     GPIO13
-- 1      GPIO5    8     GPIO15
-- 2      GPIO4    9     GPIO3
-- 3      GPIO0    10    GPIO1
-- 4      GPIO2    11    GPIO9
-- 5      GPIO14   12    GPIO10
-- 6      GPIO12      

-- ** [] D0(GPIO16) can only be used as gpio read/write. no interrupt supported. no pwm/i2c/ow supported. *


--------------------------------------
-- Wifi configuration
--------------------------------------
--wifi.setmode(wifi.STATION)
--wifi.sta.config("SSID", "PASSWD")
--wifi.sta.connect()

--------------------------------------
-- Pushingbox Device ID if switch is closed
CLOSING_DEVID = "xxxxx"

--------------------------------------
-- Device ID to use if switch is open
OPENING_DEVID = "xxxxx"

--------------------------------------
  -- Pin for the switch (input max 3.3V; no internal Pullup activated)
  SWITCH_PIN = 6

--------------------------------------
USE_TEMP_SENSOR = false
--------------------------------------
  -- ds18b20 data pin (nodemcu I/O Index)
  tempsensor_pin = 3
  -- Decimal places for sensor value
  tempsensor_precision = 1

--------------------------------------
USE_BATTERY_CHECK = false
--------------------------------------
  -- ADC reference voltage (adjust for your esp module)
  vref = 0.985
  -- Values for the resistor voltage divider
  r1 = 33000
  r2 = 10000

--------------------------------------
USE_DATE_TIME = false
--------------------------------------
  -- Webserver to get the time from
  time_server_ip = "192.168.1.123"
  -- Time offset
  time_offset = 2
  -- Translate date to german (Dec=Dez, Tue=Di,..)
  date_translate = true

--------------------------------------
USE_WIFI_STRENGTH = false
--------------------------------------
  -- SSID of the router
  SSID = ""

--------------------------------------
USE_VREG_SHUTDOWN = true
--------------------------------------
  -- pin for the shutdown signal
  vreg_shutdown_pin = 2
  
--------------------------------------
-- FAILSAVE options (worst case running time with std config(3,10): ~50 seconds)
--------------------------------------
  max_retries = 3   -- amount of retries (before doing a reset)
  time_between_requests = 10  -- how long to wait between the requests
  
  -- Reset config
  DO_A_RESET = true  -- do a (single) reset
  RESET_SIGNAL_PIN = 1  -- pin for the reset signal (will be HIGH for 1.5 seconds)

--------------------------------------
  -- Logfiles get cleared if this pin is LOW
  clear_logs_pin = 5

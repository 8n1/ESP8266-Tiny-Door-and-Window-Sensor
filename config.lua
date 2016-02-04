
--------------------------------------
-- NodeMCU I/O Index vs. ESP GPIO
--------------------------------------
-- I/O    ESP GPIO  I/O   ESP GPIO
-- 0      GPIO16    7     GPIO13
-- 1      GPIO5     8     GPIO15
-- 2      GPIO4     9     GPIO3
-- 3      GPIO0     10    GPIO1
-- 4      GPIO2     11    GPIO9
-- 5      GPIO14    12    GPIO10
-- 6      GPIO12      


----------------------------------------------------------
-- Wifi config
----------------------------------------------------------
SSID = "xxxxx"
PASSWD = "xxxxx"

----------------------------------------------------------
-- Sensor ID - Unique identifier
----------------------------------------------------------
SENSOR_ID = "1"

----------------------------------------------------------
-- The service to use (choose one and configure the corresponding "..._request.lua" script)
----------------------------------------------------------
--api_request = "arrestdb_request"
--api_request = "iftft_maker_request"
--api_request = "thingspeak_request"
api_request = "pushingbox_request"

----------------------------------------------------------
-- Pin for the switch signal
-- Pin1=GPIO5
----------------------------------------------------------
switch_pin = 1

----------------------------------------------------------
-- Pin that signals the ATtiny to shut down the vreg
-- Pin2=GPIO4
----------------------------------------------------------
shutdown_signal_pin = 2

----------------------------------------------------------
-- Activity, Error and OK LED
-- Pin5=GPIO14, Pin6=GPIO12, Pin7=GPIO13
----------------------------------------------------------
act_led_pin = 5
ok_led_pin = 6
error_led_pin = 7

----------------------------------------------------------
-- OPTIONAL FEATURES
-- To activate a feature set it to true
----------------------------------------------------------

----------------------------------------------------------
-- Get the Wifi strength (OPTIONAL)
-- note: it takes about 1-2 second longer to connect to the ap if activated
----------------------------------------------------------
GET_WIFI_STRENGTH = true

----------------------------------------------------------
-- Get the Temperature (OPTIONAL)
----------------------------------------------------------
GET_DS_TEMPERATURE = true
----------------------------------------------------------
-- ds18b20 data pin - Pin3=GPIO0
tempsensor_pin = 3

----------------------------------------------------------
-- Get the Battery voltage (OPTIONAL)
----------------------------------------------------------
GET_BATTERY_VOLTAGE = true
----------------------------------------------------------
-- resistor values for the voltage divider
r1 = 47000	-- (r5 in the schematic)
r2 = 10000	-- (r4 in the schematic)
-- internal reference voltage of the adc (turns out the reference voltage isn't exactly 1.0V)
vref = 0.984 -- reference voltage on one of my ESP-12 modules

--------------------------------------
-- Get the date and time (OPTIONAL)
--------------------------------------
USE_DATE_TIME = true
--------------------------------------
  -- Webserver to get the time from (not NTP)
  time_server_ip = "192.168.1.123"
  -- Time correction hack
  time_offset = 1
  -- Translate date to german (Dec=Dez, Tue=Di,..)
  date_translate = true

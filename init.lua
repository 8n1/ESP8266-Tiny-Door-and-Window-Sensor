--------------------------------------------

-- tiny Door and Window Sensor with Ultra Low Standby Power (<1µA)
-- Version: 0.2.1
-- Author: Johannes S. (joh.raspi)
-- Changelog: 
--  * Tempsensor added
--  * Date and Time added

-- Default running time: 3-5 seconds
-- If GET_WIFI_STRENGTH is activated: 4-6 seconds

--------------------------------------------

--------------------------------------
-- intro
print(" ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~")
print("|  tiny Door and Window Sensor v0.2  |")
print(" ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~")

--------------------------------------
-- load user config
dofile("config.lc")

-- set ssid and password
--wifi.sta.config(SSID, PASSWD)
-- configure as client only
wifi.setmode(wifi.STATION)
-- connect to the ap
wifi.sta.connect()

--------------------------------------
-- init the leds
gpio.mode(act_led_pin, gpio.OUTPUT)
gpio.write(act_led_pin, 0) 
gpio.mode(ok_led_pin, gpio.OUTPUT)
gpio.write(ok_led_pin, 0)
gpio.mode(error_led_pin, gpio.OUTPUT)
gpio.write(error_led_pin, 0)

--------------------------------------
 -- signal the ATtiny to shut down the vreg
 --------------------------------------
function vreg_shutdown()     
    -- turn off the activity led
    tmr.stop(1)
    gpio.write(act_led_pin, 0)
    
    -- print the running time
    print("~~~~~~~~~~~~~~~~~~~~~~~~~")
    print(" Timer: " ..string.format("%.2f", tmr.now()/1000/1000) .." seconds elapsed")

    -- shut down
    print(" Shuting down...")
    gpio.mode(shutdown_signal_pin, gpio.OUTPUT)
    -- after setting the pin back to LOW(0) the vreg gets turned off
    gpio.write(shutdown_signal_pin, 1)
    tmr.delay(250*1000)
    gpio.write(shutdown_signal_pin, 0)
end

--------------------------------------
-- get the battery voltage
--------------------------------------
vbat = 0
function get_battery_voltage()
    -- get the raw ADC value
    local adc_value = adc.read(0)
    -- calculate the input voltage
    local vin = vref/1024 * adc_value * (r1+r2)/r2
    -- return it (format: 0.00)
    return string.format("%.2f", vin)
end

--------------------------------------
-- get the RSSI for the currently configured AP
-- https://github.com/nodemcu/nodemcu-firmware/wiki/nodemcu_api_en#wifistagetap
--------------------------------------
rssi = 0 
quality = 0
function get_rssi()
   function listap(t)
     for bssid,v in pairs(t) do
      local ssid, l_rssi, authmode, channel = string.match(v, "([^,]+),([^,]+),([^,]+),([^,]+)")
        rssi = l_rssi
        quality = 2 * (rssi + 100)
        --print("GOT RSSI: "..rssi.." ("..quality.."%)")
     end
   end
  ssid, tmp, bssid_set, bssid=wifi.sta.getconfig() 
  scan_cfg={}
  scan_cfg.ssid=ssid 
  if bssid_set==1 then scan_cfg.bssid=bssid else scan_cfg.bssid=nil end
  scan_cfg.channel=wifi.getchannel()
  scan_cfg.show_hidden=0
  ssid, tmp, bssid_set, bssid=nil, nil, nil, nil
  wifi.sta.getap(scan_cfg, 1, listap)
end

--------------------------------------
-- ignore first reading because it is old, 
-- just read it to start a new conversion
ds_temp = 0
if GET_DS_TEMPERATURE then
    ds_ignore = true
    dofile("get_temp.lc")
end
        
--------------------------------------
-- get the wifi signal strength
if GET_WIFI_STRENGTH then
    get_rssi()
end
get_rssi = nil

--------------------------------------
-- MAIN LOOP
--------------------------------------
-- let the tempsensor finish the temp conversion
tmr.alarm(0, 800, 0, function() 
    -- get the actual temperature
    if GET_DS_TEMPERATURE then
        dofile("get_temp.lc")
    end
    -- start checking if got a ip
    tmr.alarm(0, 250, 1, function() 
        if wifi.sta.getip() == nil then
            print(".")
        else
            -- stop checking
            tmr.stop(0)
            
            -- print the ip address, the time it took to get it and the rssi
            ip_address = wifi.sta.getip()
            print(" IP address: " ..ip_address ..string.format(" (%.2fs)", tmr.now()/1000/1000))
            if GET_WIFI_STRENGTH then
                print(" RSSI: " ..rssi .." (" ..quality .."%)")
            end
    
            -- get and print the battery voltage
            if GET_BATTERY_VOLTAGE then
                vbat = get_battery_voltage()
                print(" Battery voltage: " ..vbat .."V")
            end
            get_battery_voltage = nil
            
            -- print the temperature
            if GET_DS_TEMPERATURE then
                print(" Temperature: " ..ds_temp .."'C")
            end
            GET_DS_TEMPERATURE = nil
            
            g_date = ""
            -- a small delay to let the heap recover
            tmr.alarm(0, 350, 0, function()
                if USE_DATE_TIME then 
                    print("\n Getting time...")
                    dofile("get_time.lc")
                else
                    -- launch the API Request
                    dofile(api_request ..".lc")
                    
                    -- in some rare cases the request doesn't get sent the first time,
                    -- so we try it 4 additional times with 10 seconds delay, 
                    -- just to go sure the request really gets sent.
                    max_retries = 4
                    retry_delay = 10
                    tmr.alarm(0, retry_delay*1000, 1, function()
                        -- light up the error led for 500ms
                        gpio.write(error_led_pin, 1)
                        tmr.alarm(2, 500, 0, function()
                            gpio.write(error_led_pin, 0)
                        end)
                        
                        -- try to re-send the API Request
                        print(" Re-Sending the API Request...")
                        dofile(api_request ..".lc")
        
                        -- give up after "max_retries" retries and shut down
                        max_retries = max_retries - 1
                        if max_retries == 0 then
                            tmr.stop(0)
                            print("\n -> Could not send the Request! :(")
                            vreg_shutdown() 
                        end
                    end)
                end
            end)
        end
    
        -- shut down if could not connect to the ap after 15 seconds
        if tmr.now()/1000/1000 >= 15 then
            tmr.stop(0)
            print("\n No wifi connection. (Status: " ..wifi.sta.status() ..")")     
            vreg_shutdown() 
        end
    end)
end)

-- blink/toggle the activity led
tmr.alarm(1, 800, 1, function()
    if gpio.read(act_led_pin) == 0 then
        gpio.write(act_led_pin, 1)
    else
        gpio.write(act_led_pin, 0)
    end
end)

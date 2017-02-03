--------------------------------------
-- Śends the data to a php script called ArrestDB
-- https://github.com/alixaxel/ArrestDB
--------------------------------------
-- Used Variables:
-- switch     = switch_state  = the state of the switch
-- quality    = quality       = wifi signal strength in %
-- vbat       = vbat          = battery voltage
-- temp       = ds_temp       = ds18b20 temperature
-- sensor_id  = SENSOR_ID     = unique id of the sensor

--------------------------------------
-- for a standalone test uncomment this line:
--switch_pin=6 quality=0 vbat=0 ds_temp=0 SENSOR_ID=0 vreg_shutdown=function()end


--------------------------------------
-- ArrestDB API config
--------------------------------------
-- server ip
API_IP = "xxxxx"
-- path to arrestdb.php
API_URL = "xxxxx"
-- database table to use
API_TABLE = "xxxxx"


----------------------------------------------------------------------------
----------------------------------------------------------------------------

--------------------------------------
-- read the state of the switch
--------------------------------------
if gpio.read(switch_pin) == 1 then
    switch_state = "Open"
else
    switch_state = "Closed"
end
print("\n Switch is \"" ..switch_state .."\"")
print(" Updating ArrestDB Table: " ..API_TABLE)

--------------------------------------
-- append all the data to single variable
--------------------------------------
data = "state="..switch_state .."&signal="..quality .."&vbat="..vbat .."&temperature="..ds_temp .."&sensor_id="..SENSOR_ID
--print(" Data: " ..data)

--------------------------------------
-- measure the time it takes to get a respone
local re_timer = tmr.now()
--------------------------------------
-- flag that checks if the on:receive event already got called
local got_response = false

-- create the connection
local reqConn = net.createConnection(net.TCP, 0)
-- if got a response, parse it...
reqConn:on("receive", function(reqConn, payload)
    -- ...but only if not already done that
    if not got_response then
        got_response = true
        -- search for the "200 OK" status code
        if string.find(payload, "HTTP/1.1 200 OK") then
            -- stop the retry timer and print SUCCESS
            tmr.stop(0)
            print(string.format(" -> SUCCESS (%.2fs)\n", (tmr.now()-re_timer)/1000/1000))
            -- light up the "ok" led and shut down
            gpio.write(ok_led_pin, 1)
            vreg_shutdown()
        else
            -- if the returned status code is not 200, print the response to see what happened
            -- also light up the "error" led
            gpio.write(error_led_pin, 1)
            print(payload)
            print("\n\n -> FAIL\n")
        end
    end
end)
-- send the request
reqConn:connect(80, API_IP)
reqConn:send("POST " ..API_URL ..API_TABLE .." HTTP/1.1\r\n" 
    .."Host: " ..API_IP .."\r\n" 
    .."User-Agent: NodeMCU\r\n" 
    .."Content-Type: application/x-www-form-urlencoded\r\n" 
    .."Content-Length: " ..string.len(data) .."\r\n\r\n" 
    ..data .."\r\n")
reqConn = nil

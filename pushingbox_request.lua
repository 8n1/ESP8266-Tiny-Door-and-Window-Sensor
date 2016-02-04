--------------------------------------
-- Launches a Pushingbox Scenario
-- https://www.pushingbox.com/api.php
--------------------------------------
-- Used Variables:  (note: you can use as many variables, and name them, as you like.)
-- $state$     = switch_state  = the state of the switch
-- $rssi$      = quality       = the wifi signal strength in %
-- $vbat$      = vbat          = the battery voltage
-- $temp$      = ds_temp       = ds18b20 temperature
-- $sensor_id$ = SENSOR_ID    = the unique id of the sensor

-- $date$   =   Do, 04 Feb 2016
-- $wd$     =   Do
-- $d$      =   04
-- $m$      =   Feb
-- $y$      =   2016
-- $time$   =   19:36
-- $time_n$ =   19:36:44

--------------------------------------
-- for a standalone test uncomment this line:
--switch_pin=6 quality=0 vbat=0 vreg_shutdown=function()end


--------------------------------------
-- Pushingbox Device ID of the Scenario to use if switch is open
--------------------------------------
OPENING_DEVID = "xxxxx"
--------------------------------------
-- Device ID to use if the switch is closed
--------------------------------------
CLOSING_DEVID = "xxxxx"

--------------------------------------
-- Only use a single Event
-- if set to "true" the OPENING_DEVID will always be used
--------------------------------------
USE_A_SINGLE_EVENT = false


----------------------------------------------------------------------------
----------------------------------------------------------------------------

--------------------------------------
-- read the state of the switch and decide wich Device ID to use
--------------------------------------
if gpio.read(switch_pin) == 1 then
    switch_state = "Open"
    DEVID = OPENING_DEVID
else
    switch_state = "Closed"
    DEVID = CLOSING_DEVID
end
if USE_A_SINGLE_EVENT then
    DEVID = OPENING_DEVID
end
print("\n Switch is \"" ..switch_state .."\"")
print(" Launching Pushingbox Scenario: " ..DEVID)

--------------------------------------
-- append all the data to single variable / build the GET Query String
--------------------------------------
data = "state="..switch_state .."&rssi="..quality .."&vbat="..vbat .."&temperature="..ds_temp .."&sensor_id="..SENSOR_ID
-- append date and time variables
data = data ..g_date
--print(" Data: " ..data)

--------------------------------------
-- measure the time it takes to get a respone
local re_timer = tmr.now()
--------------------------------------
-- flag that checks if the on:receive event already got called
local got_response = false

-- create the connection
local reqConn = net.createConnection(net.TCP, 0)
-- first get the ip...
reqConn:dns('api.pushingbox.com', function(reqConn, ip) 
    -- if got a response, parse it
    reqConn:on("receive", function(reqConn, payload)
        -- but only if not already done that
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
    -- ...then send the request
    reqConn:connect(80, ip)
    reqConn:send("GET /pushingbox?devid=" ..DEVID .."&" ..data .." HTTP/1.1\r\n"
        .."Host: api.pushingbox.com\r\n"
        .."User-Agent: NodeMCU\r\n"
        .."Connection: close\r\n"
        .."Accept: */*\r\n"
        .."\r\n")
end)
reqConn = nil

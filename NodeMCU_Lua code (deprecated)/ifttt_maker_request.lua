--------------------------------------
-- Launches a IFTTT Maker Event
-- https://ifttt.com/maker
--------------------------------------
-- Used Variables:  (note: IFTTT only allows the use of 3 pre-named variables)
-- value1 = switch_state  = the state of the switch
-- value2 = quality       = the wifi signal strength in %
-- value3 = vbat          = the battery voltage

--------------------------------------
-- for a standalone test uncomment this line:
--switch_pin=6 quality=0 vbat=0 vreg_shutdown=function()end


--------------------------------------
-- IFTT Maker Channel API key
--------------------------------------
MAKER_CHANNEL_KEY = "xxxxx"
--------------------------------------
-- Maker Event to use if the switch is open
--------------------------------------
OPENING_EVENT = "xxxxx"
--------------------------------------
-- Maker Event to use if the switch is closed
--------------------------------------
CLOSING_EVENT = "xxxxx"

--------------------------------------
-- Only use a single Event
-- if set to "true" the OPENING_EVENT will always be used
--------------------------------------
USE_A_SINGLE_EVENT = false


----------------------------------------------------------------------------
----------------------------------------------------------------------------

--------------------------------------
-- read the state of the switch and decide wich event to use
--------------------------------------
if gpio.read(switch_pin) == 1 then
    switch_state = "Open"
    MAKER_EVENT = OPENING_EVENT
else
    switch_state = "Closed"
    MAKER_EVENT = CLOSING_EVENT
end
if USE_A_SINGLE_EVENT then
    MAKER_EVENT = OPENING_EVENT
end
print("\n Switch is \"" ..switch_state .."\"")
print(" Launching Maker Event: " ..MAKER_EVENT)

--------------------------------------
-- append all the data to single variable / build the GET Query String
--------------------------------------
data = "value1="..switch_state .."&value2="..quality .."&value3="..vbat
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
reqConn:dns("maker.ifttt.com", function(reqConn,ip) 
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
    reqConn:send("GET /trigger/" ..MAKER_EVENT .."/with/key/" ..MAKER_CHANNEL_KEY .."?" ..data .." HTTP/1.1\r\n"
        .."Host: maker.ifttt.com\r\n"
        .."User-Agent: NodeMCU\r\n"
        .."Connection: close\r\n"
        .."Accept: */*\r\n"
        .."\r\n")
end)
reqConn = nil

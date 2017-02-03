--------------------------------------
-- Updates a Thingspeak Channels
-- https://thingspeak.com/docs
--------------------------------------
-- Used Variables:  (note: only 8 pre-named variables can be used: field1 - field8)
-- field1 = switch_state  = the state of the switch
-- field2 = quality       = the wifi signal strength in %
-- field3 = vbat          = the battery voltage
-- field4 = ds_temp       = ds18b20 temperature

--------------------------------------
-- for a standalone test uncomment this line:
--switch_pin=6 quality=0 vbat=0 ds_temp=0 vreg_shutdown=function()end


--------------------------------------
-- Thingspeak API key to use
--------------------------------------
API_KEY = "xxxxx"


----------------------------------------------------------------------------
----------------------------------------------------------------------------

--------------------------------------
-- read the state of the switch
--------------------------------------
if gpio.read(switch_pin) == 1 then
    switch_state = "1"
else
    switch_state = "0"
end
print("\n Switch is \"" ..switch_state .."\"")
print(" Updating Thingspeak Channel: " ..API_KEY)

--------------------------------------
-- append all the data to single variable / build the GET Query String
--------------------------------------
data = "&field1="..switch_state .."&field2="..quality .."&field3="..vbat .."&field4="..ds_temp
--print(" Data: " ..data)

--------------------------------------
-- measure the time it takes to get a respone
local re_timer = tmr.now()
-------------------------------------
-- flag that checks if the on:receive event already got called
local got_response = false

-- create the connection
local reqConn = net.createConnection(net.TCP, 0)
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
-- send the request
reqConn:connect(80, "184.106.153.149")
reqConn:send("GET /update?key="..API_KEY ..data .." HTTP/1.1\r\n"
    .."Host: api.thingspeak.com\r\n"
    .."User-Agent: NodeMCU\r\n"
    .."Connection: close\r\n"
    .."Accept: */*\r\n"
    .."\r\n")
reqConn = nil

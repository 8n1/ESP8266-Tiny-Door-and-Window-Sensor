-- Get time from a webserver by doing a HTTP HEAD request and parsing the response "Date" header field...
-- ...extract the date and launch the defined(devid) Pushingbox Scenario

-- create the connection
local conn = net.createConnection(net.TCP, 0)        
conn:on("receive", function(conn, payload)
    if string.find(payload, "Date: ") then
        -- get the 'Date' header field (Example: 'Date: Tue, 15 Nov 1994 08:12:31 GMT')
        res = string.sub(payload, string.find(payload, "Date: ") + 6, string.find(payload, "Date: ") + 30)
        -- extract date, time, hours, minutes and seconds and append to the Query String
        dofile("parse_date.lc")
    else
        print(payload)
        print(" -> FAIL\n")
    end
    -- launch the Request anyway (small delay to let the heap recover)
    tmr.alarm(0, 350, 0, function()
        dofile(api_request ..".lc")

        max_retries = 4
        retry_delay = 10
        tmr.alarm(0, retry_delay*1000, 1, function()
            gpio.write(error_led_pin, 1)
            tmr.alarm(2, 500, 0, function()
                gpio.write(error_led_pin, 0)
            end)
            
            print(" Re-Sending the API Request...")
            dofile(api_request ..".lc")
                        
            max_retries = max_retries - 1
            if max_retries == 0 then
                tmr.stop(0)
                print("\n -> Could not send the Request! :(")
                vreg_shutdown() 
            end
        end)
    end)
end)
conn:connect(80, time_server_ip)
conn:send("HEAD / HTTP/1.1\r\n"
   .."Host: " ..time_server_ip .."\r\n"
   .."Connection: close\r\n"
   .."Accept: */*\r\n"
   .."\r\n")
conn = nil

-- failsave timer
tmr.alarm(0, 5000, 0, function()
    print(" Time over: Could not connect to: " ..time_server_ip .." ...\n")
    print(" Trying to send the API request anyway...")
    
    dofile(api_request ..".lc")
    
    max_retries = 4
    retry_delay = 10
    tmr.alarm(0, retry_delay*1000, 1, function()
        gpio.write(error_led_pin, 1)
        tmr.alarm(2, 500, 0, function()
            gpio.write(error_led_pin, 0)
        end)
        
        print(" Re-Sending the API Request...")
        dofile(api_request ..".lc")
                    
        max_retries = max_retries - 1
        if max_retries == 0 then
            tmr.stop(0)
            print("\n -> Could not send the Request! :(")
            vreg_shutdown() 
        end
    end)
end)

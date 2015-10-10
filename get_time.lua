-- Get time from a webserver by doing a HTTP HEAD request and parsing the response "Date" header field...
-- ...extract the date and launch the defined(devid) Pushingbox Scenario

-- create the connection
local conn = net.createConnection(net.TCP, 0)        
conn:on("receive", function(conn, payload)
    if string.find(payload, "HTTP/1.1 200 OK") then
        -- get the 'Date' header field (Example: 'Date: Tue, 15 Nov 1994 08:12:31 GMT')
        res = string.sub(payload, string.find(payload, "Date: ") + 6, string.find(payload, "Date: ") + 30)
        -- extract date, time, hours, minutes and seconds and append to the Query String
        dofile("parse_date.lc")
    else
        fail_type = "get_time_api_fails"
        dofile("log_fails.lc")
        print(payload)
        print(" -> FAIL\n")
    end
    -- launch the Pushingbox scenario anyway (small delay to let the heap recover)
    tmr.alarm(0, 350, 0, function()
        print(" Launching Pushingbox Scenario...")
        fail_safe("launch_scenario.lc", "req_fails")
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
tmr.alarm(0, 3500, 0, function()
    print(" Could not connect to: " ..time_server_ip .." ...\n")
    print(" Trying to launch the Pushingbox Scenario anyway...")
    fail_safe("launch_scenario.lc", "req_fails")
end)
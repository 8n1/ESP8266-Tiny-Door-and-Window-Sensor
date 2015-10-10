-- Launches a Pushingbox Scenario with some variables, then goes into cyclic or infinite DeepSleep

-- check if the switch is opened or closed
if gpio.read(SWITCH_PIN) == 1 then
    devid = OPENING_DEVID
    print(" ...OPEN: " ..devid)
    data = data.."&state=1"
else
    devid = CLOSING_DEVID
    print(" ...CLOSED: " ..devid)
    data = data.."&state=0"
end

-- measure the time it takes to get a respone
local re_timer = tmr.now()/1000/1000

-- the last thing we append is the time it took to reach this point
data = data.."&req_time="..string.format("%.2f", re_timer)
--print("Query String: '"..data.."'")

-- check if on:receive event already got called (bugfix)
local got_response = false

-- create the connection
local dnsConn = net.createConnection(net.TCP, 0)
-- first get the ip...
dnsConn:dns('api.pushingbox.com', function(pushConn, ip) 
    -- parse the response
    pushConn:on("receive", function(conn, payload)
        if not got_response then
            got_response = true
            if string.find(payload, "HTTP/1.1 200 OK") then
                -- stop the failsafe timer
                tmr.stop(2)
                -- calculate the time it took to get the response (for debugging)
                re_timer = string.format("%.2f", tmr.now()/1000/1000-re_timer)
                -- print SUCCESS
                print(" -> SUCCESS (" ..re_timer .."s)\n")

                -- remove the did_a_reset flag(file)
                file.remove("did_a_reset")
                
                print("~~~~~~~~~~~~~~~~~~~~~~~~~")
                -- activate deep sleep
                print(" DeepSleeping...")
                dofile("deepsleep.lc")
            else
                fail_type = "api_fails"
                dofile("log_fails.lc")

                print(payload)
                print("\n -> FAIL\n")
            end
        end
    end)
    -- ...then send the request
    pushConn:connect(80, ip)
    pushConn:send("GET /pushingbox?devid=" ..devid ..data .." HTTP/1.1\r\n"
        .."Host: api.pushingbox.com\r\n"
        .."User-Agent: NodeMCU/0.9.5\r\n"
        .."Connection: close\r\n"
        .."Accept: */*\r\n"
        .."\r\n")
end)
dnsConn = nil
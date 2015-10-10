-- Read, print and append the content of all logfiles

-- logfiles to check
local types = { 
    "req_fails", 
    "fails", 
    "api_fails", 
    "get_time_api_fails", 
    "wifi_fails",
    "reset"
}

if gpio.read(clear_logs_pin) == 1 then
    -- sum up all fail counters for easier debugging
    local total_fail_count = 0
    -- loop through all logfiles, read the content and append it to the query string
    for count = 1, 6 do
        data = data .."&" ..types[count] .."="
        if file.open(types[count] .."_counter.txt", "r") then
            local counter = file.read()
            file.close()
            print(" " ..types[count] ..": " ..counter)
            data = data ..counter
            total_fail_count = total_fail_count + counter
        else
            -- set to zero if logfile does not exist
            data = data .."0"
        end
    end
    
    -- nicer looking debug output (separtes printed fails)
    if total_fail_count > 0 then print("--------------------------\n") end
    
    -- also append the total amount of fails
    data = data .."&total_fails=" ..total_fail_count
else
    print(" Clearing logfiles...\n")
    for count = 1, 6 do
        file.remove(types[count] .."_counter.txt")
    end
end

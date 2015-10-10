-- Log wifi fails to fail, increment content if file already exists

--fail_type = ""

if file.open(fail_type .."_counter.txt", "r") then
    failCount = file.read()
    file.close()
    print(" " ..fail_type ..": " ..failCount + 1)
    
    file.open(fail_type .."_counter.txt", "w+")
    file.write(failCount + 1)
    file.close()
else
    file.close()
    
    print(" -> Creating " ..fail_type .."_counter.txt")
    file.open(fail_type .."_counter.txt", "w+")
    file.write('1')
    file.close()
end

fail_type = nil
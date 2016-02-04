-- compile and remove all files in the "comp_files" array
local comp_files = { 
    "config.lua",
    "ds18b20.lua",
    "get_temp.lua",
    "parse_date.lua",
    "get_time.lua",
    "arrestdb_request.lua", 
    "ifttt_maker_request.lua", 
    "pushingbox_request.lua", 
    "thingspeak_request.lua"
}

for count = 1, 9 do
    if file.open(comp_files[count], "r") then
        file.close()
        node.compile(comp_files[count])
        file.remove(comp_files[count])
        print("File: \"" ..comp_files[count] .."\" compiled and removed.")
    end
end

-- also compile the init.lua
if file.open("init.lua", "r") then
    node.compile("init.lua")
    file.remove("init.lua")
    file.rename("init.lc", "init.lua")
    print("File: \"init.lua\" compiled.\n")
end

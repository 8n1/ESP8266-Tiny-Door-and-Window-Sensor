-- Get the temperature from a ds18b20 temperature sensor
-- https://github.com/nodemcu/nodemcu-firmware/blob/master/lua_modules/ds18b20/
-- Requires the ds18b20 module: ds18b20.lua

-- require ds18b20 module
local t = require("ds18b20")
-- setup sensor
t.setup(tempsensor_pin)

-- read temperature
ds_temp = ds18b20.read()

-- if couldn't get temperature
if ds_temp == nil then
	-- print a error message
    if ds_ignore then
        print(" -> 'Could not read ds18b20. Check wiring'\n")
    end
    -- set anyway
    ds_temp = -99
end

-- if not in ignoring mode
if ds_ignore == nil then
    ds_temp = string.format("%.1f", ds_temp)
end

-- clean up
ds_ignore = nil
t = nil
ds18b20 = nil
package.loaded["ds18b20"] = nil

-- Get the temperature from a ds18b20 temperature sensor
-- https://github.com/nodemcu/nodemcu-firmware/blob/master/lua_modules/ds18b20/
-- Requires the ds18b20 module: ds18b20.lua

-- require ds18b20 module
local t = require("ds18b20")
-- setup sensor
t.setup(tempsensor_pin)

-- read temperature
ds_temp = ds18b20.read()

-- if couldn't get temperature...
if ds_temp == nil then
    if ds_debug then
        print(" -> 'Could not read ds18b20. Check wiring'\n")
    end
    -- ...set anyway
    ds_temp = 0
end

-- if not in debugging mode, format and append to Query String
if ds_debug == nil then
    ds_temp = string.format("%." ..tempsensor_precision.. "f", ds_temp)
    data = data .."&temperatur="..ds_temp
end

-- don't forget to release it after use
ds_debug = nil
t = nil
ds18b20 = nil
package.loaded["ds18b20"] = nil

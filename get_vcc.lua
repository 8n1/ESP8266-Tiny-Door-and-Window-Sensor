-- Calculate the battery voltage and and based on that decide which scenario to launch

-- raw ADC value
local adc_value = adc.read(0)
-- voltage divider ratio
local volt_div = (r1+r2)/r2
-- valculate the battery voltage
vin = vref/1024 * adc_value * volt_div

-- format: 0.00,
vin = string.format("%.2f", vin)
-- and append to Query String
data = data.."&vbat="..vin

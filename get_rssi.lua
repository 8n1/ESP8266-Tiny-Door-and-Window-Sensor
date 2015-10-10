-- Get the wifi strength 

function listap(t)
    for k,v in pairs(t) do
        if k == SSID then
            rssi = string.sub(v, string.find(v, ",")+1, string.find(v, ",") + 3)
            quality = 2 * (rssi + 100)

            data = data.."&rssi="..rssi.."&quality="..quality
            listap = nil
            break
        end
    end
end

wifi.sta.getap(listap)

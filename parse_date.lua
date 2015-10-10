-- Extracts the date and time from the responses "Date" header field...

        -- Example: 'Wed, 15 Nov 1995 06:25:24 GMT'

        ------------------------------------------------------------------------------------------------------------
        -- Extract the (webservers) timezone
        --timezone = string.sub(res, -3)
        ------------------------------------------------------------------------------------------------------------

        ------------------------------------------------------------------------------------------------------------
        -- Extract the date, removes the time (Tue, 15 Nov 1994)
        ------------------------------------------------------------------------------------------------------------
        date = string.sub(res, 0, string.find(res, ":")-4)
        
        ------------------------------------------------------
        -- extract Year from date (1994)
        year = string.sub(date, -4)

         ------------------------------------------------------
        -- extract Day from date (15)
        day = string.sub(date, string.find(date, " ")+1, 7)

        ------------------------------------------------------
        -- extract Weekday from date (Tue)
        weekday = string.sub(date, 0, string.find(date, ",")-1)
        -- Translate to german
        if date_translate then
            weekdays_en = { "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun" }
            weekdays_de = { "Mo", "Di", "Mi", "Do", "Fr", "Sa", "So" }
            -- iterate over weekdays_en array, replace match
            for c, w in pairs(weekdays_en) do
                if w == weekday then
                    weekday = weekdays_de[c]
                    break
                end
            end
        end
        
        ------------------------------------------------------
        -- extract Month from date (Nov)
        month = string.sub(date, date.len(date)-7, date.len(date)-5)
        -- Translate to german
        if date_translate then
            months_en = { "Jan", "Mar", "May", "Oct", "Dec" }
            months_de = { "Jan", "Mar", "Mai", "Okt", "Dez" }
            -- iterate over months_en array, replace match
            for c, m in pairs(months_en) do
                if m == month then
                    month = months_de[c]
                    break
                end
            end
        end

        ------------------------------------------------------------------------------------------------------------
        -- Format date: "Do, 24 Dez 2015"
        date = weekday..", "..day.." "..month.." "..year
        ---------------------------
        print(" -> Date: " ..date)

        ------------------------------------------------------
        -- sanitize/replace whitespaces and append the date to the Query String
        date = date:gsub("% ", "+")
        ---------------------------
        data = data.."&date="..date.."&wd="..weekday.."&d="..day.."&m="..month.."&y="..year
        
        ------------------------------------------------------
        -- clean up
        date, year = nil
        month, months_en, months_de = nil
        weekday, weekdays_en, weekdays_de = nil

        
        ------------------------------------------------------------------------------------------------------------
        -- extract the time (08:12:31)
        ------------------------------------------------------------------------------------------------------------
        time = string.sub(res, string.find(res, ":") - 2, string.find(res, ":") + 5)
        -- then the hours, minutes and seconds
        hours = string.sub(time, 0, 2)
        minutes = string.sub(time, 4, 5)
        seconds = string.sub(time, 7, 8)

        ------------------------------------------------------
        -- do a small time correction (TODO: Winter/Sommerzeit)
        hours = (hours+time_offset) % 24
        -- re-add the "trunculated" leading zero
        hours = string.format("%02d", hours)

        -- Sommerzeit: +1 - So. 29. März 2015, um 02:00 MEZ (letzter Sonntagmorgen im März)
        -- Winterzeit: -1 - So. 25. Oktober 2015, um 03:00 MESZ (letzter Sonntagmorgen im Oktober)
        
        ------------------------------------------------------------------------------------------------------------
        -- Format time: 00:00
        time = hours..":"..minutes
        -- Format time_n: 00:00:00
        time_n = time..":"..seconds
        ---------------------------
        print(" -> Time: " ..time_n.. "\n")
        
        ------------------------------------------------------
        -- Append the time to the Query String
        data = data.."&time="..time.."&time_n="..time_n
        
        ------------------------------------------------------
        -- clean up
        res = nil
        time, time_n = nil
        hours, minutes, seconds = nil

-- blink.lua -- which frames have no picture in them?
--
-- A frame is black when the beam spends (almost) none of it unblanked. Tap
-- both TIA registers -- VSYNC ($00) delimits frames, VBLANK ($01) opens and
-- closes the picture -- and SUM the time VBLANK is clear. "Was it ever
-- cleared" is not enough: a bank that clears it in the last few cycles of a
-- frame it never drew would count as a picture.
if os.getenv("DRIVE_LUA") then dofile(os.getenv("DRIVE_LUA")) end
local sp = manager.machine.devices[":maincpu"].spaces["program"]
local n, lit, opened, run, runs, hist = 0, 0, nil, 0, 0, {}
local bankat = {}
local FRAME = 1 / 59.92

local function now() return manager.machine.time:as_double() end

_G._bl1 = sp:install_write_tap(0x01, 0x01, "vblank", function(off, data)
    if (data & 0x02) == 0 then
        opened = opened or now()
    elseif opened then
        lit = lit + (now() - opened)
        opened = nil
    end
end)
_G._bl2 = sp:install_write_tap(0x00, 0x00, "vsync", function(off, data)
    if (data & 0x02) == 0 then return end
    n = n + 1
    if opened then lit = lit + (now() - opened); opened = now() end
    local drew = lit > FRAME * 0.25
    if n > 1 then
        if not drew then
            run = run + 1
            bankat[#bankat + 1] = sp:readv_u8(0x1F0E)
        elseif run > 0 then
            runs = runs + 1
            hist[run] = (hist[run] or 0) + 1
            print(string.format("BLINK #%d: %d black frame(s) ending f%d, banks %s",
                                runs, run, n, table.concat(bankat, ",")))
            run, bankat = 0, {}
        end
    end
    lit = 0
    if n % 900 == 0 then
        local out = {}
        for k, v in pairs(hist) do out[#out + 1] = k .. " frames x" .. v end
        print("BLINK total " .. runs .. ": " .. table.concat(out, ", "))
    end
end)

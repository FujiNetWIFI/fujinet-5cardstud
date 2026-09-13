-- lobbyjump.lua -- walk the lobby cursor and report the frame length each
-- press costs. A redraw that does not fit the vblank budget lengthens its
-- frame, and a frame of a different length is a picture in a different place.
local FRAME, LINE = 1 / 59.92, (1 / 59.92) / 262
local n, last, hist, pressed = 0, nil, {}, 0
local sp = manager.machine.devices[":maincpu"].spaces["program"]
local up, down = manager.machine.ioport.ports[":SWB"], nil
for tag, p in pairs(manager.machine.ioport.ports) do
    for fname, f in pairs(p.fields) do
        if fname:match("Up") then up = f end
        if fname:match("Down") then down = f end
    end
end
_G._lj = sp:install_write_tap(0x00, 0x00, "vsync", function(off, data)
    if (data & 0x02) == 0 then return end
    local t = manager.machine.time:as_double()
    n = n + 1
    if last then
        local lines = math.floor((t - last) / LINE + 0.5)
        hist[lines] = (hist[lines] or 0) + 1
        if lines > 266 then
            print(string.format("JUMP: %d lines at f%d, bank %d",
                                lines, n, sp:readv_u8(0x1F0E)))
        end
    end
    last = t
    -- once the lobby is up, nudge the cursor every 20 frames
    if n > 240 and sp:readv_u8(0x1F0E) == 0 then
        local phase = n % 20
        if phase == 0 then
            if down then down:set_value(1) end
            pressed = pressed + 1
        elseif phase == 6 then
            if down then down:set_value(0) end
        end
    end
    if n % 400 == 0 then
        local keys = {}
        for k in pairs(hist) do keys[#keys+1] = k end
        table.sort(keys)
        local out = {}
        for _, k in ipairs(keys) do out[#out+1] = k .. ":" .. hist[k] end
        print("LOBBY presses=" .. pressed .. " LINES " .. table.concat(out, " "))
    end
end)

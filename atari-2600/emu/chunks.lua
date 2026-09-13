-- chunks.lua -- how many frames does a recompose take, and do any of them
-- overrun? Counts consecutive VSYNCs emitted while BANKCMP is live.
if os.getenv("DRIVE_LUA") then dofile(os.getenv("DRIVE_LUA")) end
local FRAME, LINE = 1 / 59.92, (1 / 59.92) / 262
local sp = manager.machine.devices[":maincpu"].spaces["program"]
local n, last, lastbank, run, hist, over, steps = 0, nil, 0, 0, {}, 0, {}
_G._ck = sp:install_write_tap(0x00, 0x00, "vsync", function(off, data)
    if (data & 0x02) == 0 then return end
    local t = manager.machine.time:as_double()
    n = n + 1
    local bank = sp:readv_u8(0x1F0E)
    -- only INTERIOR compose frames: the first one's gap is the handover, not
    -- a chunk, and it is long by construction.
    if last and bank == 5 and lastbank == 5 then
        local lines = math.floor((t - last) / LINE + 0.5)
        if lines > 262 then
            over = over + 1
            local step = sp:readv_u8(0x92)      -- CCSTEP borrows CDINP
            steps[step] = (steps[step] or 0) + 1
        end
    end
    if bank == 5 then
        run = run + 1
    elseif run > 0 then
        hist[run] = (hist[run] or 0) + 1
        run = 0
    end
    last, lastbank = t, bank
    if n % 900 == 0 then
        local keys = {}
        for k in pairs(hist) do keys[#keys + 1] = k end
        table.sort(keys)
        local out = {}
        for _, k in ipairs(keys) do out[#out + 1] = k .. " frames x" .. hist[k] end
        local so = {}
        for k, v in pairs(steps) do so[#so + 1] = "step " .. k .. " x" .. v end
        print("CHUNKS " .. table.concat(out, ", ") .. " | long frames " ..
              over .. " at " .. table.concat(so, ", "))
    end
end)

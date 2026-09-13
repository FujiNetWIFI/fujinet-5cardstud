-- blank.lua -- how long does a network poll stop the picture for?
--
-- Not by counting frames spent in the network bank: that measures where the
-- program is, not whether anything is on screen, and it reads the same before
-- and after the bank learned to draw. Not by sampling the screen either --
-- screen:pixel() reads a bitmap MAME updates on its own schedule, and it
-- reports no change through a blank that is plainly visible.
--
-- What is unambiguous is the program's own VSYNC. Tap the write, measure the
-- gap to the previous one, and anything past one frame is a frame the client
-- did not draw. A stopped kernel emits no VSYNC at all, so the gap IS the
-- length of the flash.
--
--   DRIVE_LUA=emu/drive.lua ./run.sh 5card blank
--
-- Without DRIVE_LUA it boots to the lobby and sits there, which polls once.
if os.getenv("DRIVE_LUA") then dofile(os.getenv("DRIVE_LUA")) end

local FRAME = 1 / 59.92
local LINE  = FRAME / 262
local hist  = {}
local last, lastbank, n, bad, sum, worst = nil, 0, 0, 0, 0, 0

local sp = manager.machine.devices[":maincpu"].spaces["program"]
_G._blank = sp:install_write_tap(0x00, 0x00, "vsync", function(off, data, mask)
    if (data & 0x02) == 0 then return end          -- VSYNC going ON only
    local t = manager.machine.time:as_double()
    local bank = sp:readv_u8(0x1F0E)
    if last then
        local gap = t - last
        n = n + 1
        local lines = math.floor(gap / LINE + 0.5)
        hist[lines] = (hist[lines] or 0) + 1
        if gap > FRAME * 1.02 then
            bad = bad + 1
            sum = sum + gap
            if gap > worst then worst = gap end
            print(string.format("GAP #%d: %d lines (%.2f frames) at %.2fs, "
                                .. "bank %d -> %d",
                                bad, lines, gap / FRAME, t, lastbank, bank))
        end
        if n % 900 == 0 then
            local keys = {}
            for k in pairs(hist) do keys[#keys+1] = k end
            table.sort(keys)
            local out = {}
            for _, k in ipairs(keys) do
                out[#out+1] = string.format("%d:%d", k, hist[k])
            end
            print("LINES " .. table.concat(out, " "))
        end
        if n % 900 == 0 then
            print(string.format("GAP: %d of %d frames dropped, mean %.0fms, "
                                .. "worst %.0fms", bad, n,
                                bad > 0 and sum / bad * 1000 or 0,
                                worst * 1000))
        end
    end
    last, lastbank = t, bank
end)

-- lobbyjump.lua -- walk the lobby cursor and report what each press costs the
-- frame it lands in. A redraw that does not fit the vblank budget pushes the
-- first drawn line down by the overrun, and that is what "the screen bounces"
-- is: the picture starting later inside its own frame.
--
-- IT ALSO CHECKS THERE IS SOMETHING ON SCREEN, and that is not belt and
-- braces: a frame-length test passes trivially on a blank screen, and one did.
-- A bad edit left the lobby calling the cursor-only redraw on entry as well,
-- so the list was never composed at all -- and this test reported 108 presses
-- and every frame at 262, which was true and meaningless. The check reads the
-- cartridge's own text planes at $1800, which is what the kernel draws from.
--
-- THE PORT TAG IS NOT OPTIONAL. An earlier cut of this walked every port
-- looking for a field whose name matched "Down", which finds P1 Down and then
-- P2 Down, and pairs() order decided which one it kept. When it kept P2 the
-- presses went nowhere, the lobby sat idle, and the test reported 108 presses
-- and no jumps at all -- from a screen that bounces badly in front of a human.
local JOY = ":joyport1:joy:JOY"

local FRAME = 1 / 59.92
local LINE = FRAME / 262
local n, last, hist, pressed = 0, nil, {}, 0
local sp = manager.machine.devices[":maincpu"].spaces["program"]
local port = manager.machine.ioport.ports[JOY]
local down = port and port.fields["P1 Down"]
local up   = port and port.fields["P1 Up"]
if not down or not up then error("no " .. JOY .. " / P1 Up+Down") end

local function inked()
    local n = 0
    for plane = 0, 5 do
        for i = 0, 127, 3 do
            if sp:readv_u8(0x1800 + plane * 128 + i) ~= 0 then n = n + 1 end
        end
    end
    return n
end

local blankframes = 0

_G._lj = sp:install_write_tap(0x00, 0x00, "vsync", function(off, data)
    if (data & 0x02) == 0 then return end
    local t = manager.machine.time:as_double()
    n = n + 1
    if last then
        local lines = math.floor((t - last) / LINE + 0.5)
        hist[lines] = (hist[lines] or 0) + 1
        if lines > 266 then
            print(string.format("JUMP: %d lines at f%d, bank %d", lines, n,
                                sp:readv_u8(0x1F0E)))
        end
    end
    last = t
    if n > 240 and sp:readv_u8(0x1F0E) == 0 and inked() < 12 then
        blankframes = blankframes + 1
    end
    -- once the lobby is up, nudge the cursor every 20 frames
    if n > 240 and sp:readv_u8(0x1F0E) == 0 then
        -- ALTERNATE. Holding one direction walks the cursor to the end of a
        -- five-table list in four presses and every press after that is a
        -- no-op, so a down-only test reports four jumps and then silence.
        local phase = n % 20
        local f = ((n // 20) % 2 == 0) and down or up
        if phase == 0 then
            f:set_value(1)
            pressed = pressed + 1
        elseif phase == 6 then
            down:set_value(0)
            up:set_value(0)
        end
    end
    if n % 400 == 0 then
        local keys = {}
        for k in pairs(hist) do keys[#keys + 1] = k end
        table.sort(keys)
        local out = {}
        for _, k in ipairs(keys) do out[#out + 1] = k .. ":" .. hist[k] end
        print("LOBBY presses=" .. pressed .. " ink=" .. inked() ..
              " blankframes=" .. blankframes .. " LINES " ..
              table.concat(out, " "))
    end
end)

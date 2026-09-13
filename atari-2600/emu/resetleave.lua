-- resetleave.lua -- RESET opens the in-game menu, and LEAVE there gives the
-- seat up.
--
-- Not soft_reset() -- that is emu/resettest.lua, and a different thing
-- entirely. This is SWCHB bit 0, the switch the program READS, which restarts
-- nothing and means whatever the client decides. It used to mean "cold start,
-- back to the lobby", which abandoned the seat: no /leave went out and the
-- table kept listing a player who had gone.
--
-- The whole path, in one go: press RESET at the table, land in the menu, walk
-- the cursor to LEAVE, fire, and then watch for /leave followed by /tables and
-- a lobby with a list actually on it.
--
-- Requests are caught with a WRITE TAP on CDREQ, not sampled once a frame: a
-- transaction can start and finish between two frames, and a sampling version
-- of this test saw only the /tables that followed.
if os.getenv("DRIVE_LUA") then dofile(os.getenv("DRIVE_LUA")) end

local CDREQ, CDSEL, CDNPLR, BANK = 0x8F, 0xAA, 0xAC, 0x1F0E
local RQTABLE, RQLEAVE = 0, 3
local B_GAME, B_MENU, B_LOBBY = 1, 3, 0
local MILEAVE = 2

local sp = manager.machine.devices[":maincpu"].spaces["program"]
local joy = manager.machine.ioport.ports[":joyport1:joy:JOY"]
local down, fire = joy.fields["P1 Down"], joy.fields["P1 Button 1"]
local rst
for name, f in pairs(manager.machine.ioport.ports[":SWB"].fields) do
    if name == "Reset Game" then rst = f end
end
if not (down and fire and rst) then error("missing an input field") end

local function inked()
    local c = 0
    for p = 0, 5 do
        for i = 0, 127, 3 do
            if sp:readv_u8(0x1800 + p * 128 + i) ~= 0 then c = c + 1 end
        end
    end
    return c
end

local function say(f, ...) print("RESETLEAVE: " .. string.format(f, ...)) end

local n, phase, mark, seen = 0, "wait", 0, {}
local sawmenu = false

_G._rlq = sp:install_write_tap(CDREQ, CDREQ, "cdreq", function(off, data)
    if phase ~= "wait" and seen[#seen] ~= data then
        seen[#seen + 1] = data
        say("request %d", data)
    end
end)

-- PRESS ON ENTRY. A helper that only acted when n == mark never pressed
-- anything: the frame that sets mark has already run its branch, so by the
-- time the new phase is looked at, n is mark + 1 and the equality is gone.
-- This test reported "RESET did not open the menu" from a rig that never
-- touched the switch.
local function press(field, next_phase)
    field:set_value(1)
    mark, phase = n, next_phase
end
local function release(field, hold, next_phase)
    if n >= mark + hold then
        field:set_value(0)
        mark, phase = n, next_phase
        return true
    end
    return false
end

local function verdict()
    local leave, tables = false, false
    for _, r in ipairs(seen) do
        if r == RQLEAVE then leave = true end
        if r == RQTABLE and leave then tables = true end
    end
    local ink = inked()
    say("menu=%s leave=%s tables=%s lobby ink=%d",
        tostring(sawmenu), tostring(leave), tostring(tables), ink)
    if sawmenu and leave and tables and ink > 12 then say("PASS")
    else say("FAIL") end
    manager.machine:exit()
end

_G._rl = emu.add_machine_frame_notifier(function()
    n = n + 1
    local bank = sp:readv_u8(BANK)

    if phase == "wait" then
        if n > 420 and bank == B_GAME and sp:readv_u8(CDNPLR) > 0 then
            say("at the table, pressing RESET at f%d", n)
            press(rst, "press")
        end

    elseif phase == "press" then
        release(rst, 8, "menu")

    elseif phase == "menu" then
        if bank == B_MENU then
            if not sawmenu then
                sawmenu = true
                say("menu is up at f%d, ink=%d", n, inked())
            end
            if sp:readv_u8(CDSEL) == MILEAVE then
                say("LEAVE selected")
                press(fire, "fire")
            elseif n >= mark + 16 then
                press(down, "cursor")
            end
        elseif n > mark + 180 then
            say("FAIL: RESET did not open the menu")
            manager.machine:exit()
        end

    elseif phase == "cursor" then
        release(down, 8, "menu")

    elseif phase == "fire" then
        release(fire, 8, "leaving")

    elseif phase == "leaving" then
        if bank == B_LOBBY and n > mark + 20 then
            verdict()
        elseif n > mark + 900 then
            say("FAIL: never reached the lobby")
            manager.machine:exit()
        end
    end
end)

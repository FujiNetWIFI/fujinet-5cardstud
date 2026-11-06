-- smoke.lua -- read the Atari 7800 screen out of the MARIA engine's map and
-- give a verdict.
--
-- Modelled on support/nes/smoke.lua and
-- fujinet-firmware/pico/atari-7800/emu/mtext.lua. The engine keeps a byte per
-- cell (tile * 2) at the start of the cart's RAM and the palette selects three
-- pages on; a tile is decoded through the table src/atari7800/mkchr.py writes
-- (support/atari7800/tilemap.lua). Card art reads as "#". Under a row, "~"
-- marks the cells drawn on black (palette 1): the status bar and the
-- highlights.
--
--   FCS_TILEMAP path to support/atari7800/tilemap.lua (required)
--   FCS_AT      emulated seconds to settle before sampling; measured from the
--               end of FCS_SCRIPT when there is one
--   FCS_SETTLE  emulated seconds before the first press (default 8)
--   FCS_SCRIPT  comma-separated actions to play first: fire b2 reset select
--               pause up down left right, select+pause (held together), any
--               of them as name*S to hold for S seconds, waitN to idle N
--               emulated seconds (default 1), ?TEXT to wait (up to FCS_UNTIL
--               seconds, default 60) until TEXT is on screen (?A|B for either),
--               shot to print the screen and save a snapshot
--   FCS_EXPECT  substring that must appear on screen; sets the exit verdict
--   FCS_QUIET   only print the verdict, not the screen

local AT      = tonumber(os.getenv("FCS_AT") or "8")
local EXPECT  = os.getenv("FCS_EXPECT")
if EXPECT == "" then EXPECT = nil end   -- make/env hand through an empty string
local QUIET   = os.getenv("FCS_QUIET")
local TILEMAP = os.getenv("FCS_TILEMAP")
local SCRIPT  = os.getenv("FCS_SCRIPT")
local UNTIL   = tonumber(os.getenv("FCS_UNTIL") or "60")

local COLS, ROWS = 32, 24
local MAP, ATTR = 0x4000, 0x4300

local tilemap

local function mem()
    return manager.machine.devices[":maincpu"].spaces["program"]
end

local function read_screen()
    local m = mem()
    local lines, dark = {}, {}
    for row = 0, ROWS - 1 do
        local chars, marks, any = {}, {}, false
        for col = 0, COLS - 1 do
            local t = m:read_u8(MAP + row * COLS + col) >> 1
            chars[#chars + 1] = tilemap[t] or "?"
            local d = m:read_u8(ATTR + row * COLS + col) ~= 0
            marks[#marks + 1] = d and "~" or " "
            any = any or d
        end
        lines[#lines + 1] = table.concat(chars)
        dark[row + 1] = any and table.concat(marks) or false
    end
    return lines, dark
end

local function screen_text()
    local lines = read_screen()
    return table.concat(lines, "\n")
end

local function on_screen(alternatives)
    local text = screen_text()
    for want in alternatives:gmatch("[^|]+") do
        if text:find(want, 1, true) then return true end
    end
    return false
end

local function print_screen(title)
    local lines, dark = read_screen()
    print(string.format("---- %s (%.1f s) ----", title, manager.machine.time:as_double()))
    print("+--------------------------------+")
    for i, l in ipairs(lines) do
        print("|" .. l .. "|")
        if dark[i] then print(" " .. dark[i]) end
    end
    print("+--------------------------------+")
end

local FIELDS = {
    up = { ":JOYSTICKS", "P1 Up" }, down = { ":JOYSTICKS", "P1 Down" },
    left = { ":JOYSTICKS", "P1 Left" }, right = { ":JOYSTICKS", "P1 Right" },
    fire = { ":BUTTONS", "P1 Button 1" }, b2 = { ":BUTTONS", "P1 Button 2" },
    reset = { ":CONSOLE", "Reset" }, select = { ":CONSOLE", "Select" },
    pause = { ":CONSOLE", "Pause" },
}

local function press(action, on)
    for name in action:gsub("%*.*$", ""):gmatch("[^+]+") do
        local f = FIELDS[name]
        if f == nil then error("smoke.lua: unknown action '" .. action .. "'") end
        local field = manager.machine.ioport.ports[f[1]].fields[f[2]]
        if field == nil then error("smoke.lua: no field " .. f[2]) end
        field:set_value(on and 1 or 0)
    end
end

local function shot(title)
    print_screen(title)
    local scr = manager.machine.screens[":screen"]
    if scr then scr:snapshot() end
end

-- SETTLE has to outlast the client's start-up round trips -- the appkey reads
-- and the first table fetch. A press delivered while it is still inside one
-- of those is missed by whatever loop is not yet running.
local HOLD, GAP = 0.30, 0.45
local SETTLE = tonumber(os.getenv("FCS_SETTLE") or "8")

local script = {}
if SCRIPT then
    for a in SCRIPT:gmatch("[^,]+") do script[#script + 1] = a:match("^%s*(.-)%s*$") end
end

-- MAME re-runs this on every reset; the state lives in _G.
if _G.fcs_state == nil then
    _G.fcs_state = { fired = false, step = 1, down = false, t_next = SETTLE, shots = 0 }
end
local st = _G.fcs_state

local function next_step(now, delay)
    st.step = st.step + 1
    st.t_next = now + delay
    if st.step > #script then st.t_sample = st.t_next + AT end
end

-- The subscription has to stay in a live global: add_machine_frame_notifier
-- hands back an RAII token, and letting it fall out of scope unsubscribes at
-- the next Lua collection.
_G.fcs_sub = emu.add_machine_frame_notifier(function ()
    if st.fired then return end
    local now = manager.machine.time:as_double()

    if tilemap == nil then
        local chunk, err = loadfile(TILEMAP or "")
        if chunk == nil then
            print("fcs: FAIL tilemap: " .. tostring(err))
            st.fired = true
            manager.machine:exit()
            return
        end
        tilemap = chunk()
    end

    if st.step <= #script then
        if now < st.t_next then return end
        local action = script[st.step]
        local n = action:match("^wait(%d*)$")
        if n then
            next_step(now, (n == "" and 1 or tonumber(n)))
        elseif action:sub(1, 1) == "?" then
            st.t_wait = st.t_wait or now
            if on_screen(action:sub(2)) then
                print(string.format("fcs: saw %q at %.1f s", action:sub(2), now))
                st.t_wait = nil
                next_step(now, 0)
            elseif now - st.t_wait > UNTIL then
                print_screen("timed out")
                print(string.format("fcs: FAIL never saw %q", action:sub(2)))
                st.fired = true
                manager.machine:exit()
            end
        elseif action == "shot" then
            st.shots = st.shots + 1
            shot("shot " .. st.shots)
            next_step(now, 0)
        elseif st.down then
            press(action, false)
            st.down = false
            next_step(now, GAP)
        else
            press(action, true)
            st.down = true
            st.t_next = now + (tonumber(action:match("%*([%d.]+)$")) or HOLD)
        end
        return
    end

    if now < (st.t_sample or AT) then return end
    st.fired = true

    if not QUIET then print_screen("sample") end

    -- The map keeps whatever was last drawn; the PC and the mailbox status
    -- page say whether the client is still running and what the last
    -- transaction said.
    do
        local cpu = manager.machine.devices[":maincpu"]
        local m = mem()
        print(string.format("fcs: PC=%04X ACKSEQ=%02X STATUS=%02X ERR=%02X",
            cpu.state["PC"].value, m:read_u8(0x0C00), m:read_u8(0x0C01), m:read_u8(0x0C02)))
    end

    if EXPECT then
        if screen_text():find(EXPECT, 1, true) then
            print("fcs: PASS")
        else
            print("fcs: FAIL expected " .. string.format("%q", EXPECT))
        end
    end
    manager.machine:exit()
end)

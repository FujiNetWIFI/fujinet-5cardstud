-- smoke.lua -- read the SMS screen out of the name table and give a verdict.
--
-- Modelled on support/nes/smoke.lua. Text tiles sit at their ASCII codes, but
-- card art and the colour variants do not, so a name table entry is decoded
-- through the tile -> character table src/sms/mktiles.py writes
-- (support/sms/tilemap.lua): card art reads "#", a rank its letter, a suit
-- s/c/d/h and a chip "o".
--
--   FCS_TILEMAP path to support/sms/tilemap.lua (required)
--   FCS_AT      emulated seconds to settle before sampling; measured from the
--               end of FCS_SCRIPT when there is one
--   FCS_SETTLE  emulated seconds before the first press (default 8)
--   FCS_SCRIPT  comma-separated actions to play first, e.g. "b1,wait10,b2+b1":
--               b1 b2 pause up down left right, a chord of them joined by "+",
--               waitN to idle N emulated seconds (default 1), shot to print
--               the screen and snapshot it
--   FCS_EXPECT  substring that must appear on screen; sets the exit verdict
--   FCS_QUIET   only print the verdict, not the screen

local AT      = tonumber(os.getenv("FCS_AT") or "8")
local EXPECT  = os.getenv("FCS_EXPECT")
if EXPECT == "" then EXPECT = nil end   -- make/env hand through an empty string
local QUIET   = os.getenv("FCS_QUIET")
local TILEMAP = os.getenv("FCS_TILEMAP")
local SCRIPT  = os.getenv("FCS_SCRIPT")

local COLS, ROWS, NAME_TABLE = 32, 24, 0x3800

local tilemap
local function load_tilemap()
    if TILEMAP == nil then return "FCS_TILEMAP is not set" end
    local chunk, err = loadfile(TILEMAP)
    if chunk == nil then return err end
    tilemap = chunk()
    return nil
end

local function vram()
    for tag, dev in pairs(manager.machine.devices) do
        if tag:match("vdp$") and dev.spaces["videoram"] then
            return dev.spaces["videoram"]
        end
    end
end

local function read_screen()
    local v = vram()
    if v == nil then return nil, "no VDP videoram space" end
    local lines = {}
    for row = 0, ROWS - 1 do
        local chars = {}
        for col = 0, COLS - 1 do
            local a = NAME_TABLE + (row * COLS + col) * 2
            local t = v:read_u8(a) | ((v:read_u8(a + 1) & 1) << 8)
            chars[#chars + 1] = tilemap[t] or "?"
        end
        lines[#lines + 1] = table.concat(chars)
    end
    return lines, nil
end

local function show()
    local lines, err = read_screen()
    if lines == nil then
        emu.print_info("fcs: FAIL " .. err)
        return nil
    end
    if not QUIET then
        print("+--------------------------------+")
        for _, l in ipairs(lines) do print("|" .. l .. "|") end
        print("+--------------------------------+")
    end
    return lines
end

local function snapshot()
    local scr = manager.machine.screens[":screen"]
    if scr then scr:snapshot() end
end

local BTN = { b1 = "P1 Button 1", b2 = "P1 Button 2",
              up = "P1 Up", down = "P1 Down", left = "P1 Left", right = "P1 Right" }

-- SETTLE has to outlast the client's start-up round trips -- the appkey reads
-- and the first table fetch. A press delivered while it is still inside one
-- of those is seen by the edge detector and dropped by whatever loop is not
-- yet running.
local HOLD, GAP = 0.20, 0.45
local SETTLE = tonumber(os.getenv("FCS_SETTLE") or "8")

local function field(name)
    if name == "pause" then
        return manager.machine.ioport.ports[":PAUSE"].fields["Pause"]
    end
    local f = BTN[name] and manager.machine.ioport.ports[":ctrl1:mspad:JOYPAD"].fields[BTN[name]]
    if f == nil then error("smoke.lua: unknown action '" .. name .. "'") end
    return f
end

-- Each action becomes timed events: { field, value } presses, { wait = s },
-- { shot = true }. A chord such as "b2+b1" presses left to right and lets go
-- right to left, holding each step for HOLD.
local events = {}
local function ev(e) events[#events + 1] = e end
if SCRIPT then
    for a in SCRIPT:gmatch("[^,%s]+") do
        local n = a:match("^wait(%d*)$")
        if n then
            ev({ wait = n == "" and 1 or tonumber(n) })
        elseif a == "shot" then
            ev({ shot = true })
        else
            local keys = {}
            for k in a:gmatch("[^+]+") do keys[#keys + 1] = k end
            for i = 1, #keys do ev({ key = keys[i], value = 1 }); ev({ wait = HOLD }) end
            for i = #keys, 1, -1 do ev({ key = keys[i], value = 0 }); ev({ wait = HOLD }) end
            ev({ wait = GAP })
        end
    end
end

if _G.fcs_state == nil then
    _G.fcs_state = { fired = false, step = 1, t_next = SETTLE }
end
local st = _G.fcs_state

local err = load_tilemap()
if err then
    emu.print_info("fcs: FAIL " .. err)
end

-- The subscription has to stay in a live global: add_machine_frame_notifier
-- hands back an RAII token, and letting it fall out of scope unsubscribes at
-- the next Lua collection.
_G.fcs_sub = emu.add_machine_frame_notifier(function ()
    if st.fired or tilemap == nil then return end
    local now = manager.machine.time:as_double()

    while st.step <= #events and now >= st.t_next do
        local e = events[st.step]
        st.step = st.step + 1
        if e.wait then
            st.t_next = now + e.wait
        elseif e.shot then
            print(string.format("fcs: shot at %.1fs", now))
            show()
            snapshot()
        else
            field(e.key):set_value(e.value)
        end
        if st.step > #events then st.t_sample = st.t_next + AT end
    end
    if st.step <= #events then return end

    if now < (st.t_sample or AT) then return end
    st.fired = true

    local lines = show()
    snapshot()

    -- "The screen looks right" is not proof on its own: VRAM keeps whatever
    -- was last drawn. The PC and the mailbox status page say whether the
    -- client is still running and what the last transaction said.
    do
        local cpu = manager.machine.devices[":maincpu"]
        local mem = cpu.spaces["program"]
        print(string.format("fcs: PC=%04X ACKSEQ=%02X STATUS=%02X ERR=%02X at %.1fs",
            cpu.state["PC"].value,
            mem:readv_u8(0xB400), mem:readv_u8(0xB401), mem:readv_u8(0xB402), now))
    end

    if EXPECT and lines then
        local joined = table.concat(lines, "\n")
        if joined:find(EXPECT, 1, true) then
            print("fcs: PASS")
        else
            print("fcs: FAIL expected " .. string.format("%q", EXPECT))
        end
    end
    manager.machine:exit()
end)

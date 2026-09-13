-- resettest.lua -- restart the 6507 mid-session and prove the conversation
-- continues.
--
-- This connector has NO RESET LINE to the cartridge. A restart re-zeroes
-- nothing on the cart side: it keeps running with its sequence number where
-- it was. So a client that counted sequences in RAM would restart at 1,
-- collide with a sequence already answered, and have every later transaction
-- dropped in silence -- which is why FNGO derives the next one from the
-- cartridge's own ACKSEQ.
--
-- PASS requires the post-reset sequence to be exactly ONE more than the value
-- it started from: the client talked again, and it counted from the cart.
--
-- soft_reset(), NOT the "Reset Game" ioport field. That field is the console
-- SWITCH, which on this machine is a bit in a RIOT register the program reads
-- -- pressing it reboots nothing at all. (The client does act on it, by
-- choice; this tests the restart itself.)
--
-- soft_reset RE-RUNS this script, so every bit of state lives in _G or the
-- harness restarts with the machine and waits forever for a reset it has
-- already done.

local ACKSEQ = 0x1F00
local BANK   = 0x1F0E

_G._rt = _G._rt or { n = 0, phase = "warm", before = nil }

local function say(f, ...) print("RESET: " .. string.format(f, ...)) end

_G._reset = _G._reset or emu.add_machine_frame_notifier(function()
    local t = _G._rt
    t.n = t.n + 1
    if t.n < 240 then return end
    local sp = manager.machine.devices[":maincpu"].spaces["program"]

    if t.phase == "warm" then
        t.before = sp:readv_u8(ACKSEQ)
        if t.before == 0 then return end        -- nothing has talked yet
        say("ACKSEQ is %02X before the reset, bank %d",
            t.before, sp:readv_u8(BANK))
        t.phase = "after"
        t.n = 0
        manager.machine:soft_reset()
        return
    end

    if t.phase == "after" then
        -- Long enough to boot and make at least one transaction.
        if t.n < 1200 then return end
        local now = sp:readv_u8(ACKSEQ)
        local want = (t.before % 255) + 1       -- 0 is reserved: 255 wraps to 1
        say("ACKSEQ is %02X after the reset (want %02X), bank %d",
            now, want, sp:readv_u8(BANK))
        manager.machine.video:snapshot()
        if now == want then
            say("PASS -- the sequence came from the cartridge, not from RAM")
        else
            say("FAIL -- the client did not derive its sequence from the cart")
        end
        t.phase = "exit"
        t.n = 0
        return
    end

    -- The exit must not share a frame with the snapshot or the file never
    -- flushes.
    if t.phase == "exit" and t.n > 20 then manager.machine:exit() end
end)

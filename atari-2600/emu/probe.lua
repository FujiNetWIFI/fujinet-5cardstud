-- probe.lua -- dump what the client actually did.
--
-- `readv_u8` (not read_u8) is the one that goes through the live memory map,
-- which is what a banked cartridge needs.
local n = 0
local ENT = { [0]="COLD", "LOBBY", "GCOLD", "GAME", "FETCH", "TABLE",
              "MENU", "LEAVE", "NAME" }

local function hex(v) return string.format("%02X", v) end

_G._probe_token = emu.add_machine_frame_notifier(function()
    n = n + 1
    local every = tonumber(os.getenv("PROBE_EVERY") or "120")
    if n % every ~= 0 then return end
    local sp = manager.machine.devices[":maincpu"].spaces["program"]

    local ent = sp:readv_u8(0xBB)
    print(string.format(
        "PROBE f%-5d bank=%d ent=%-6s err=%02X step=%02X cnt=%d nplr=%d " ..
        "round=%d act=%02X nmv=%d sel=%d poll=%d ack=%02X rx=%d",
        n, sp:readv_u8(0x1F0E), ENT[ent] or ("?" .. ent),
        sp:readv_u8(0x9A), sp:readv_u8(0x9B), sp:readv_u8(0xAB),
        sp:readv_u8(0xAC), sp:readv_u8(0xAD), sp:readv_u8(0xAE),
        sp:readv_u8(0xAF), sp:readv_u8(0xAA), sp:readv_u8(0xA7),
        sp:readv_u8(0x1F00),
        sp:readv_u8(0x1F04) + 256 * sp:readv_u8(0x1F05)))

    if os.getenv("PROBE_REPLY") then
        io.write("PROBE reply ")
        for i = 0, 47 do io.write(hex(sp:readv_u8(0x1B00 + i))) end
        print()
    end
end)

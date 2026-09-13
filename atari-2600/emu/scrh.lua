-- scrh.lua -- does the emulated screen change shape? MAME's TIA follows the
-- program's frame length, so a frame of a different height is a picture in a
-- different place -- which is what "the screen jumps" is.
if os.getenv("DRIVE_LUA") then dofile(os.getenv("DRIVE_LUA")) end
local n, seen, last = 0, {}, nil
_G._scrh = emu.add_machine_frame_notifier(function()
    n = n + 1
    local s = manager.machine.screens[":screen"]
    local h = s.height
    local key = string.format("%dx%d", s.width, h)
    seen[key] = (seen[key] or 0) + 1
    if last and key ~= last then
        print(string.format("SHAPE f%d: %s -> %s (bank %d)", n, last, key,
              manager.machine.devices[":maincpu"].spaces["program"]:readv_u8(0x1F0E)))
    end
    last = key
    if n % 900 == 0 then
        for k, v in pairs(seen) do print("SHAPE " .. k .. " x" .. v) end
    end
end)

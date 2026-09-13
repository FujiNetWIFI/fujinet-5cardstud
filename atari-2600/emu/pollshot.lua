-- pollshot.lua -- snapshot eight consecutive frames across a poll, so the
-- panel's vertical position can be compared frame by frame. "The screen
-- jumps" is that position changing; nothing else about the picture moves.
if os.getenv("DRIVE_LUA") then dofile(os.getenv("DRIVE_LUA")) end
local n, armed, shots = 0, false, 0
_G._ps = emu.add_machine_frame_notifier(function()
    n = n + 1
    local sp = manager.machine.devices[":maincpu"].spaces["program"]
    local bank = sp:readv_u8(0x1F0E)
    if n > 400 and bank == 2 and not armed and shots == 0 then armed = true end
    if armed and shots < 8 then
        manager.machine.video:snapshot()
        shots = shots + 1
        print(string.format("SHOT %d at f%d bank %d", shots, n, bank))
    elseif shots >= 8 then
        manager.machine:exit()
    end
end)

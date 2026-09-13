-- ports.lua -- dump every ioport field name, so a harness can name its
-- presses correctly. Copied from fujinet-firmware/pico/atari-2600/emu.
--
-- Worth having: the joystick is ":joyport1:joy:JOY" with fields like
-- "P1 Button 1", while the console switches are ":SWB" with "Select Game" and
-- "Reset Game" -- and a wrong field name is a silent no-op, not an error.
local n = 0
_G._ports = emu.add_machine_frame_notifier(function()
    n = n + 1
    if n ~= 5 then return end
    for tag, port in pairs(manager.machine.ioport.ports) do
        for fname, f in pairs(port.fields) do
            print(string.format("%-28s %s", tag, fname))
        end
    end
    manager.machine:exit()
end)

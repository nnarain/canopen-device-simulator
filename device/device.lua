function OnInit()
    ConfigureTimer(1000)
    print('Device initialized')
end

function OnSync(cnt)
    print('Sync ' .. cnt)
end

function OnTick()
    print('Timer tick')
end

function OnWrite(index, subindex)
    value = GetU32(0x4000, 0x00)
    print('Object ' .. string.format("%4x", index) .. "." .. subindex .. " was updated to " .. string.format("%08x", value))
    SetU32(0x4001, 0x00, value + 1)
end

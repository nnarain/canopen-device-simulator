ticks = 0

function OnInit()
    -- Configure the internal periodic timer
    ConfigureTimer(1000)
    -- Configure/Override the device heartbeat producer time
    ConfigureHeartbeat(100)

    print('Device initialized')
end

function OnSync(cnt)
    print('Sync ' .. cnt)
end

function OnTick()
    print('Timer tick ' .. ticks)
    ticks = ticks + 1

    if ticks == 10 then
        Emcy(0x8130, 0x01)
    elseif ticks == 20 then
        Emcy(0x0000, 0x00)
    end
end

function OnWrite(index, subindex)
    value = GetU32(0x4000, 0x00)
    print('Object ' .. string.format("%4x", index) .. "." .. subindex .. " was updated to " .. string.format("%08x", value))
    SetU32(0x4001, 0x00, value + 1)
end

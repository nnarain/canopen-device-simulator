a = nil

function OnInit()
    a = 'hello'
    print('OnInit')
end

function OnSync(cnt)
    print('Sync ' .. cnt)
end

function OnWrite(index, subindex)
    value = GetU32(0x4000, 0x00)
    print('Object ' .. string.format("%4x", index) .. "." .. subindex .. " was updated to " .. string.format("%08x", value))
    SetU32(0x4001, 0x00, value + 1)
end

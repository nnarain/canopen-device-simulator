a = nil

function OnInit()
    a = 'hello'
    print('OnInit')
end

function OnSync(cnt)
    print('Sync ' .. cnt)
end

function OnWrite(index, subindex)
    print('Object ' .. string.format("%4x", index) .. ":" .. subindex .. " was updated to " .. GetU32(0x4000, 0x00))
end

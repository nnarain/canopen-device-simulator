-- Device objects
Register('foo', 0x4000, 0x00, ObjectType.UINT32)
Register('bar', 0x4001, 0x00, ObjectType.UINT32)
Register('baz', 0x4002, 0x00, ObjectType.UINT32)

function OnInit()
    ConfigureTimer(1000)

    ObjectCallback(0x4000, 0x00, FooUpdated)

    print('Foo: ' .. string.format('0x%4X', objects.foo))
    print('Bar: ' .. string.format('0x%4X', objects.bar))
    print('Baz: ' .. string.format('0x%4X', objects.baz))
end

function OnTick()
    objects.bar = objects.bar + 1
    print('bar: ' .. objects.bar .. ', foo: ' .. objects.foo)
end

function FooUpdated()
    print('Foo was updated')
end

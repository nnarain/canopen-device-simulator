-- Test Device

require('objects')
require('device_helper')

function OnInit()
    ConfigureTimer(1000)

    ObjectCallback(0x4000, 0x00, FooUpdated)

    print('Foo: ' .. string.format('0x%4X', objects.foo))
    print('Bar: ' .. string.format('0x%4X', objects.bar))
    print('Baz: ' .. string.format('0x%4X', objects.baz))

    VeryUsefulHelper()
end

function OnTick()
    objects.bar = objects.bar + 1
    print('bar: ' .. objects.bar .. ', foo: ' .. objects.foo)
end

function FooUpdated()
    print('Foo was updated')
end

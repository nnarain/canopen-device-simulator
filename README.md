# CANopen Device Simulator

This project aims to create scriptable CANopen slave nodes to simulate hardware devices.

Using the [Lely CANopen API](https://opensource.lely.com/canopen/), common CANopen device events are detected and trigger event handlers in a specified Lua script.

Build
-----

Follow installation instructions for Lely: https://opensource.lely.com/canopen/docs/installation/

Install dependencies using your preferred method:

* Lua
* sol2
* Boost

```bash
$ cd to/project/directory
$ mkdir build && cd build
$ cmake ..
$ cmake --build .
```


Usage
-----

In order to run the scripted CANopen device you will need the following:

* An EDS/DCF file of your device
* A Lua script representing the device behavior you wish to simulate

A device script represents the device itself. A few functions are provided to access the device life cycle.

```lua
-- Called when slave node is initialized
function OnInit()
    -- Configure a period timer
    ConfigureTimer(1000)

    print('Device Initialized')
end

-- Called at the interval specified by `ConfigureTimer`
function OnTick()
    print('Called every 1s')
end
```

Scripts operate on CANopen objects. To access object data, first register the object with the variable you want to use.

```lua
Register('foo', 0x4000, 0x00, ObjectType.UINT32)

-- ...

function OnTick()
    print('Foo: ' .. objects.foo)
end
```

Package paths can be used to `require` other Lua packages. Use the following environment variable:

```
export COSIM_LIB_PATH=/path/to/package/?.lua
```

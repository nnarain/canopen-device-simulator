# CANopen Device Simulator

This project aims to create scriptable CANopen slave nodes to simulate hardware devices.

Using the [Lely CANopen API](https://opensource.lely.com/canopen/), common CANopen device events are detected and trigger event handlers in a specified Lua script.

Build
-----

Follow installation instructions for Lely: https://opensource.lely.com/canopen/docs/installation/

Install dependencies using your prefered method:

* Lua
* sol2
* Boost

```bash
$ cd to/project/directory
$ mkdir build && cd build
$ cmake ..
$ cmake --build .
```




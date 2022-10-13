#include <lely/ev/loop.hpp>
#include <lely/io2/linux/can.hpp>
#include <lely/io2/posix/poll.hpp>
#include <lely/io2/sys/io.hpp>
#include <lely/io2/sys/timer.hpp>
#include <lely/io2/sys/sigset.hpp>

#include <lely/coapp/slave.hpp>

#include <sol/sol.hpp>

#include <functional>

using namespace lely;

class SimulatedSlave : public canopen::BasicSlave {
public:
    SimulatedSlave(io::TimerBase& timer, io::CanChannelBase& chan,
            const std::string& dcf_txt,
            const std::string& dcf_bin, uint8_t id,
            const std::string& script) : canopen::BasicSlave{timer, chan, dcf_txt, dcf_bin, id}
    {
        // Register additional callbacks
        // OnSync()

        // Initialize the Lua scripting environment
        lua.open_libraries(
            sol::lib::base,
            sol::lib::string,
            sol::lib::math,
            sol::lib::os,
            sol::lib::table
        );
        lua.script_file(script);

        // OD Access Functions
        lua.set_function("GetU32", [this](const uint16_t idx, const uint8_t subidx) { return getU32(idx, subidx); });
    }

    virtual void OnInit() {
        lua["OnInit"]();
    }

protected:
    void OnSync(uint8_t cnt, const time_point&) noexcept override {
        lua["OnSync"](cnt);
    }

    // This function gets called every time a value is written to the local object
    // dictionary by an SDO or RPDO.
    void OnWrite(uint16_t idx, uint8_t subidx) noexcept override {
        // if (idx == 0x4000 && subidx == 0) {
        //     // Read the value just written to object 4000:00, probably by RPDO 1.
        //     uint32_t val = (*this)[0x4000][0];
        //     // Copy it to object 4001:00, so that it will be sent by the next TPDO.
        //     (*this)[0x4001][0] = val;
        // }
        // lua.script("OnWrite()")
        lua["OnWrite"](idx, subidx);
    }

private:
    template<typename T>
    sol::lua_value getObject(const uint16_t idx, const uint8_t subidx) {
        const T value = (*this)[idx][subidx];
        return value;
    }

    sol::lua_value getU32(const uint16_t idx, const uint8_t subidx) {
        return getObject<uint32_t>(idx, subidx);
    }

    sol::state lua;
};

int main() {
    // Initialize the I/O library. This is required on Windows, but a no-op on
    // Linux (for now).
    io::IoGuard io_guard;

    // Create an I/O context to synchronize I/O services during shutdown.
    io::Context ctx;
    // Create an platform-specific I/O polling instance to monitor the CAN bus, as
    // well as timers and signals.
    io::Poll poll(ctx);
    // Create a polling event loop and pass it the platform-independent polling
    // interface. If no tasks are pending, the event loop will poll for I/O
    // events.
    ev::Loop loop(poll.get_poll());
    // I/O devices only need access to the executor interface of the event loop.
    auto exec = loop.get_executor();

    // Create a timer using a monotonic clock, i.e., a clock that is not affected
    // by discontinuous jumps in the system time.
    io::Timer timer(poll, exec, CLOCK_MONOTONIC);

    // Create a virtual SocketCAN CAN controller and channel, and do not modify
    // the current CAN bus state or bitrate.
    io::CanController ctrl("vcan0");
    io::CanChannel chan(poll, exec);

    chan.open(ctrl);

    // Setup slave device
    // canopen::BasicSlave slave(timer, chan, "cpp-slave.eds", "", 2);
    SimulatedSlave slave{timer, chan, "device/device.eds", "", 2, "device/device.lua"};
    slave.OnInit();

    // Create a signal handler.
    io::SignalSet sigset(poll, exec);
    // Watch for Ctrl+C or process termination.
    sigset.insert(SIGHUP);
    sigset.insert(SIGINT);
    sigset.insert(SIGTERM);

    // Submit a task to be executed when a signal is raised. We don't care which.
    sigset.submit_wait([&](int /*signo*/) {
        // If the signal is raised again, terminate immediately.
        sigset.clear();
        // Perform a clean shutdown.
        ctx.shutdown();
    });

    slave.Reset();

    // Run the event loop until no tasks remain (or the I/O context is shut down).
    loop.run();

  return 0;
}
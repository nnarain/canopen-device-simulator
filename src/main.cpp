#include <lely/ev/loop.hpp>
#include <lely/io2/linux/can.hpp>
#include <lely/io2/posix/poll.hpp>
#include <lely/io2/sys/io.hpp>
#include <lely/io2/sys/timer.hpp>
#include <lely/io2/sys/sigset.hpp>
#include <lely/coapp/slave.hpp>

#include <boost/program_options.hpp>

#include <functional>
#include <iostream>

#include "simulated_slave.hpp"

using namespace lely;

namespace po = boost::program_options;

int main(int argc, char* argv[]) {
    po::options_description desc{"Options"};
    desc.add_options()
        ("help,h", "Help")
        ("interface,i", po::value<std::string>()->default_value("vcan0"), "CAN interface")
        ("node,n", po::value<int>(), "Node ID")
        ("model,m", po::value<std::string>(), "EDS file")
        ("script,s", po::value<std::string>(), "Lua Script");

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help"))
    {
        std::cout << desc << "\n";
        return 0;
    }

    const auto interface = vm["interface"].as<std::string>();
    const auto node_id = (uint8_t)vm["node"].as<int>();
    const auto model = vm["model"].as<std::string>();
    const auto script = vm["script"].as<std::string>();

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
    io::CanController ctrl(interface.c_str());
    io::CanChannel chan(poll, exec);

    chan.open(ctrl);

    // Setup slave device
    SimulatedSlave slave{timer, chan, model, "", node_id, script};
    slave.OnInit();

    // Create a signal handler.
    io::SignalSet sigset(poll, exec);
    // Watch for Ctrl+C or process termination.
    sigset.insert(SIGHUP);
    sigset.insert(SIGINT);
    sigset.insert(SIGTERM);

    // Submit a task to be executed when a signal is raised. We don't care which.
    sigset.submit_wait([&](int /*signo*/) {
        // Inform the slave it should shutdown
        slave.Shutdown();
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
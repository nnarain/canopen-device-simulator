//
// simulated_slave.cpp
//
// @author Natesh Narain <nnaraindev@gmail.com>
// @date May 08 2023
//

#include "simulated_slave.hpp"

#include <cstdlib>

SimulatedSlave::SimulatedSlave(lely::io::TimerBase& timer, lely::io::CanChannelBase& chan,
        const std::string& dcf_txt,
        const std::string& dcf_bin, uint8_t id,
        const std::string& script) : lely::canopen::BasicSlave{timer, chan, dcf_txt, dcf_bin, id}
{
    // Initialize the Lua scripting environment
    lua.open_libraries(
        sol::lib::base,
        sol::lib::string,
        sol::lib::math,
        sol::lib::os,
        sol::lib::table,
        sol::lib::package
    );
    setupLibraryPath(lua);

    // User types
    lua["ObjectType"] = lua.create_table_with(
        "INT8", ObjectType::INT8,
        "INT16", ObjectType::INT16,
        "INT32", ObjectType::INT32,
        "INT64", ObjectType::INT64,
        "UINT8", ObjectType::UINT8,
        "UINT16", ObjectType::UINT16,
        "UINT32", ObjectType::UINT32,
        "UINT64", ObjectType::UINT64
    );

    // Register `this` class as the object dictionary accessor
    lua.new_usertype<SimulatedSlave>("CanOpenDevice",
        sol::meta_function::index,
        &SimulatedSlave::readObject,
        sol::meta_function::new_index,
        &SimulatedSlave::writeObject
    );
    lua["objects"] = this;

    lua.set_function("Register", &SimulatedSlave::registerObject, this);
    lua.set_function("ObjectCallback", &SimulatedSlave::setupObjectCallback, this);

    // EMCY generation
    lua.set_function("Emcy", &SimulatedSlave::Emcy, this);

    // Device configuration functions
    lua.set_function("ConfigureTimer", &SimulatedSlave::configurePeriodicTimer, this);
    lua.set_function("ConfigureHeartbeat", &SimulatedSlave::configureHeartbeat, this);

    // Load the script
    lua.script_file(script);
}

void SimulatedSlave::setupLibraryPath(sol::state&)
{
    const std::string env_paths = std::string{std::getenv("COSIM_LIB_PATH")};

    const std::string package_path = lua["package"]["path"];

    lua["package"]["path"] = package_path + (!package_path.empty() ? ";" : "") + env_paths;
}

void SimulatedSlave::OnInit()
{
    lua["OnInit"]();
}

void SimulatedSlave::Shutdown()
{
    shutting_down_ = true;
}

void SimulatedSlave::OnSync(uint8_t cnt, const time_point&) noexcept
{
    lua["OnSync"](cnt);
}

void SimulatedSlave::OnWrite(uint16_t idx, uint8_t subidx) noexcept
{
    // lua["OnWrite"](idx, subidx);
    CanOpenObjectId id{idx, subidx};

    if (object_callbacks_.find(id) != object_callbacks_.end())
    {
        object_callbacks_[id]();
    }
}

void SimulatedSlave::Emcy(uint16_t error_code, uint8_t error_register)
{
    // TODO(nnarain): Support vendor data
    Error(error_code, error_register, nullptr);
}

sol::object SimulatedSlave::readObject(sol::stack_object key, sol::this_state L)
{
    const auto object_name = key.as<std::string>();
    return object_getters_[object_name](L);
}

void SimulatedSlave::writeObject(sol::stack_object key, sol::stack_object value, sol::this_state)
{
    const auto object_name = key.as<std::string>();
    object_setters_[object_name](value);
}

void SimulatedSlave::registerObject(const std::string& name, uint16_t index, uint8_t subindex, ObjectType object_type)
{
    object_getters_[name] = createGetter(index, subindex, object_type);
    object_setters_[name] = createSetter(index, subindex, object_type);
}

void SimulatedSlave::setupObjectCallback(uint16_t idx, uint8_t subidx, sol::function fn)
{
    object_callbacks_[{idx, subidx}] = fn;
}

SimulatedSlave::ObjectGetter SimulatedSlave::createGetter(const uint16_t index, const uint8_t subindex, ObjectType object_type)
{
    switch(object_type)
    {
        case ObjectType::UINT8:
            return createGetter<uint8_t>(index, subindex);
        case ObjectType::UINT16:
            return createGetter<uint16_t>(index, subindex);
        case ObjectType::UINT32:
            return createGetter<uint32_t>(index, subindex);
        case ObjectType::UINT64:
            return createGetter<uint64_t>(index, subindex);
        default:
            throw std::invalid_argument{"Invalid object type"};
    }
}

SimulatedSlave::ObjectSetter SimulatedSlave::createSetter(const uint16_t index, const uint8_t subindex, ObjectType object_type)
{
    switch(object_type)
    {
        case ObjectType::UINT8:
            return createSetter<uint8_t>(index, subindex);
        case ObjectType::UINT16:
            return createSetter<uint16_t>(index, subindex);
        case ObjectType::UINT32:
            return createSetter<uint32_t>(index, subindex);
        case ObjectType::UINT64:
            return createSetter<uint64_t>(index, subindex);
        default:
            throw std::invalid_argument{"Invalid object type"};
    }
}

// -----------------------------------------------------------------------------------------------------------------
// Device configuration
// -----------------------------------------------------------------------------------------------------------------

void SimulatedSlave::configurePeriodicTimer(const uint32_t duration_ms)
{
    periodic_timer_duration_ = std::chrono::milliseconds(duration_ms);
    startPeriodicTimer();
}

void SimulatedSlave::configureHeartbeat(const uint16_t heartbeat_ms)
{
    // TODO(nnarain): Doesn't seem to update. Maybe needs to be in pre-op?
    (*this)[0x1017][0x00] = heartbeat_ms;
}

void SimulatedSlave::startPeriodicTimer()
{
    if (!shutting_down_)
    {
        SubmitWait(periodic_timer_duration_, std::bind(&SimulatedSlave::timerCallback, this));
    }
}

void SimulatedSlave::timerCallback()
{
    lua["OnTick"]();
    startPeriodicTimer();
}

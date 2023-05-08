//
// simulated_slave.hpp
//
// @author Natesh Narain <nnaraindev@gmail.com>
// @date Apr 10 2023
//

#pragma once

#include <lely/coapp/slave.hpp>
#include <lely/io2/sys/io.hpp>

#include <sol/sol.hpp>

#include <chrono>
#include <functional>
#include <memory>
#include <iostream>

/**
 * Index / subindex pair
*/
struct CanOpenObjectId
{
    CanOpenObjectId(uint16_t index, uint8_t subindex) : index{index}, subindex{subindex} {}

    uint32_t unique_key() const
    {
        return (index << 16) | subindex;
    }

    bool operator<(const CanOpenObjectId& other)
    {
        return (*this).unique_key() < other.unique_key();
    }

    uint16_t index{0};
    uint16_t subindex{0};
};

/**
 * @brief Object Types
*/
enum class ObjectType
{
    INT8,
    INT16,
    INT32,
    INT64,
    UINT8,
    UINT16,
    UINT32,
    UINT64
};

class ObjectBase
{

};

template<typename T>
class Object : public ObjectBase
{
public:
};

/**
 * \brief CANopen Slave with Lua backend
*/
class SimulatedSlave : public lely::canopen::BasicSlave
{
    using ObjectGetter = std::function<sol::object(sol::this_state&)>;
    using ObjectSetter = std::function<void(sol::stack_object&)>;

public:
    SimulatedSlave(lely::io::TimerBase& timer, lely::io::CanChannelBase& chan,
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
            sol::lib::table
        );

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

        // EMCY generation
        lua.set_function("Emcy", &SimulatedSlave::Emcy, this);

        // Device configuration functions
        lua.set_function("ConfigureTimer", &SimulatedSlave::configurePeriodicTimer, this);
        lua.set_function("ConfigureHeartbeat", &SimulatedSlave::configureHeartbeat, this);

        // Load the script
        lua.script_file(script);
    }

    virtual void OnInit() {
        lua["OnInit"]();
    }

    virtual void Shutdown()
    {
        shutting_down_ = true;
    }

protected:
    // -----------------------------------------------------------------------------------------------------------------
    // Life Cycle
    // -----------------------------------------------------------------------------------------------------------------

    void OnSync(uint8_t cnt, const time_point&) noexcept override {
        lua["OnSync"](cnt);
    }

    // This function gets called every time a value is written to the local object
    // dictionary by an SDO or RPDO.
    void OnWrite(uint16_t idx, uint8_t subidx) noexcept override {
        lua["OnWrite"](idx, subidx);
    }

    // Invoke an EMCY
    void Emcy(uint16_t error_code, uint8_t error_register)
    {
        // TODO(nnarain): Support vendor data
        Error(error_code, error_register, nullptr);
    }

private:
    // -----------------------------------------------------------------------------------------------------------------
    // Object Dictionary Access
    // -----------------------------------------------------------------------------------------------------------------

    sol::object readObject(sol::stack_object key, sol::this_state L)
    {
        const auto object_name = key.as<std::string>();
        return object_getters_[object_name](L);
    }

    void writeObject(sol::stack_object key, sol::stack_object value, sol::this_state)
    {
        const auto object_name = key.as<std::string>();
        object_setters_[object_name](value);
    }

    void registerObject(const std::string& name, uint16_t index, uint8_t subindex, ObjectType object_type)
    {
        object_getters_[name] = createGetter(index, subindex, object_type);
        object_setters_[name] = createSetter(index, subindex, object_type);
    }

    ObjectGetter createGetter(const uint16_t index, const uint8_t subindex, ObjectType object_type) {
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

    template<typename T>
    ObjectGetter createGetter(const uint16_t index, const uint8_t subindex) {
        return [this, index, subindex](sol::this_state& L) {
            return sol::object(L, sol::in_place, getObject<T>(index, subindex));
        };
    }

    ObjectSetter createSetter(const uint16_t index, const uint8_t subindex, ObjectType object_type) {
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

    template<typename T>
    ObjectSetter createSetter(const uint16_t index, const uint8_t subindex) {
        return [this, index, subindex](sol::stack_object& value) {
            setObject<T>(index, subindex, value);
        };
    }

    /**
     * \brief Get OD object of type T
    */
    template<typename T>
    sol::lua_value getObject(const uint16_t idx, const uint8_t subidx) {
        const T value = (*this)[idx][subidx];
        return value;
    }

    /**
     * \brief Set OD object of type T
    */
    template<typename T>
    void setObject(const uint16_t idx, const uint8_t subidx, const sol::stack_object& value) {
        (*this)[idx][subidx] = value.as<T>();
    }

    // -----------------------------------------------------------------------------------------------------------------
    // Device configuration
    // -----------------------------------------------------------------------------------------------------------------

    void configurePeriodicTimer(const uint32_t duration_ms)
    {
        periodic_timer_duration_ = std::chrono::milliseconds(duration_ms);
        startPeriodicTimer();
    }

    void configureHeartbeat(const uint16_t heartbeat_ms)
    {
        // TODO(nnarain): Doesn't seem to update. Maybe needs to be in pre-op?
        (*this)[0x1017][0x00] = heartbeat_ms;
    }

    // -----------------------------------------------------------------------------------------------------------------
    // Internal timer
    // -----------------------------------------------------------------------------------------------------------------
    void startPeriodicTimer()
    {
        if (!shutting_down_)
        {
            SubmitWait(periodic_timer_duration_, std::bind(&SimulatedSlave::timerCallback, this));
        }
    }

    void timerCallback()
    {
        lua["OnTick"]();
        startPeriodicTimer();
    }

    sol::state lua;

    bool shutting_down_{false};
    std::chrono::milliseconds periodic_timer_duration_;

    std::map<std::string, ObjectGetter> object_getters_;
    std::map<std::string, ObjectSetter> object_setters_;
};

//
// simulated_slave.hpp
//
// @author Natesh Narain <nnaraindev@gmail.com>
// @date Apr 10 2023
//

#pragma once

#include "canopen_id.hpp"

#include <lely/coapp/slave.hpp>
#include <lely/io2/sys/io.hpp>

#include <sol/sol.hpp>

#include <chrono>
#include <functional>
#include <memory>
#include <iostream>
#include <map>



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
            const std::string& script);
    virtual ~SimulatedSlave() = default;

    /**
     * Signal initialization step to the script
    */
    virtual void OnInit();
    /**
     * Shutdown
    */
    virtual void Shutdown();

protected:
    // -----------------------------------------------------------------------------------------------------------------
    // Life Cycle
    // -----------------------------------------------------------------------------------------------------------------

    void OnSync(uint8_t cnt, const time_point&) noexcept override;

    // This function gets called every time a value is written to the local object
    // dictionary by an SDO or RPDO.
    void OnWrite(uint16_t idx, uint8_t subidx) noexcept override;

    // Invoke an EMCY
    void Emcy(uint16_t error_code, uint8_t error_register);

private:
    // -----------------------------------------------------------------------------------------------------------------
    // Object Dictionary Access
    // -----------------------------------------------------------------------------------------------------------------

    sol::object readObject(sol::stack_object key, sol::this_state L);
    void writeObject(sol::stack_object key, sol::stack_object value, sol::this_state);

    void registerObject(const std::string& name, uint16_t index, uint8_t subindex, ObjectType object_type);

    void setupObjectCallback(uint16_t idx, uint8_t subidx, sol::function fn);

    ObjectGetter createGetter(const uint16_t index, const uint8_t subindex, ObjectType object_type);

    template<typename T>
    ObjectGetter createGetter(const uint16_t index, const uint8_t subindex) {
        return [this, index, subindex](sol::this_state& L) {
            return sol::object(L, sol::in_place, getObject<T>(index, subindex));
        };
    }

    ObjectSetter createSetter(const uint16_t index, const uint8_t subindex, ObjectType object_type);

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

    void configurePeriodicTimer(const uint32_t duration_ms);

    void configureHeartbeat(const uint16_t heartbeat_ms);

    // -----------------------------------------------------------------------------------------------------------------
    // Internal timer
    // -----------------------------------------------------------------------------------------------------------------
    void startPeriodicTimer();
    void timerCallback();

    sol::state lua;

    bool shutting_down_{false};
    std::chrono::milliseconds periodic_timer_duration_;

    std::map<std::string, ObjectGetter> object_getters_;
    std::map<std::string, ObjectSetter> object_setters_;

    std::map<CanOpenObjectId, sol::function> object_callbacks_;
};

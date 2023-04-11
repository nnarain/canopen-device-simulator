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

/**
 * \brief CANopen Slave with Lua backend
*/
class SimulatedSlave : public lely::canopen::BasicSlave
{
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
        lua.script_file(script);

        // EMCY generation
        lua.set_function("Emcy", &SimulatedSlave::Emcy, this);

        // OD Access Functions
        lua.set_function("GetU32", &SimulatedSlave::getU32, this);
        lua.set_function("SetU32", &SimulatedSlave::setU32, this);

        // Device configuration functions
        lua.set_function("ConfigureTimer", &SimulatedSlave::configurePeriodicTimer, this);
        lua.set_function("ConfigureHeartbeat", &SimulatedSlave::configureHeartbeat, this);
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
    void setObject(const uint16_t idx, const uint8_t subidx, const sol::lua_value& value) {
        (*this)[idx][subidx] = value.as<T>();
    }

    /**
     * \brief Get OD object as U32
    */
    sol::lua_value getU32(const uint16_t idx, const uint8_t subidx) {
        return getObject<uint32_t>(idx, subidx);
    }

    /**
     * \brief Set OD object as U32
    */
    void setU32(const uint16_t idx, const uint8_t subidx, const sol::lua_value& value) {
        setObject<uint32_t>(idx, subidx, value);
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
};

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

        // OD Access Functions
        lua.set_function("GetU32", &SimulatedSlave::getU32, this);
        lua.set_function("SetU32", &SimulatedSlave::setU32, this);

        // Device configuration functions
        lua.set_function("ConfigureTimer", &SimulatedSlave::configurePeriodicTimer, this);
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
        lua["OnWrite"](idx, subidx);
    }

private:

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

    void startPeriodicTimer()
    {
        SubmitWait(periodic_timer_duration_, std::bind(&SimulatedSlave::timerCallback, this));
    }

    void timerCallback()
    {
        lua["OnTick"]();
        startPeriodicTimer();
    }

    sol::state lua;
    std::chrono::milliseconds periodic_timer_duration_;
};

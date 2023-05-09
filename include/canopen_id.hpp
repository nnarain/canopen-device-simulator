//
// canopen_id.hpp
//
// @author Natesh Narain <nnaraindev@gmail.com>
// @date May 08 2023
//

#pragma once

#include <cstdint>

/**
 * @brief Index / subindex pair
*/
struct CanOpenObjectId
{
    CanOpenObjectId(uint16_t index, uint8_t subindex) : index{index}, subindex{subindex} {}

    uint32_t unique_key() const
    {
        return (index << 16) | subindex;
    }

    bool operator<(const CanOpenObjectId& other) const
    {
        return (*this).unique_key() < other.unique_key();
    }

    uint16_t index{0};
    uint16_t subindex{0};
};

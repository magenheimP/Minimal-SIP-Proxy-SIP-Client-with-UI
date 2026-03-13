//
// Created by lazarstani on 3/12/26.
//

#ifndef SIPTRAININGPROJECT_TYPES_H
#define SIPTRAININGPROJECT_TYPES_H
#pragma once

#include <string>
#include <cstdint>

namespace common {

    struct RawPacket
    {
        std::string data;
        std::string ip;
        uint16_t port;
    };

}
#endif //SIPTRAININGPROJECT_TYPES_H
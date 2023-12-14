#pragma once

#include "string_view"
#include "gui.hpp"
#include "networking.hpp"

namespace hnoker
{
    void start_listener(const std::string_view connector_ip, const uint16_t connector_port, Gui& gui);
}



#include "listener.hpp"
#include "logging.hpp"
#include <functional>
#include <array>
#include <string_view>
#include <cstdint>
#include <span>

namespace hnoker
{
    static void connect_to_connector(std::string_view ip)
    {
    }

    void start_listener()
    {
        INFO("Starting listener");

        std::array<char, 1024> read_buffer;
        std::array<char, 1024> write_buffer;

        std::function send_message = [](std::span<char> read_buffer, std::span<char> write_buffer, const std::string& ip, const std::uint16_t& port) -> bool
        {
            return true;
        };

    }

}

#include "listener.hpp"
#include "logging.hpp"
#include "functional"
#include "message_types.hpp"
#include "networking.hpp"
#include "player.hpp"

#include <functional>
#include <array>
#include <string_view>
#include <cstdint>
#include <span>

namespace hnoker
{
    static void send_message_to_leader(Message& msg, network& net, const std::string_view leader_ip, const uint16_t leader_port, std::span<char> rb, std::span<char> wb)
    {
        std::function message = [&msg](std::span<char> read_buf, std::span<char> write_buf, const std::string& ip, std::uint16_t port) -> bool
        {
            write_message_to_buffer(write_buf, msg);
            return true;
        };
        net.async_connect_server(leader_ip, leader_port, rb, wb, message);
    }

    static bool message_handler(Message& msg)
    {
        switch (msg.type)
        {
            case MessageType::CONTROL_MUSIC:
                INFO("Listener recieved CONTROL_MUSIC");
                break;
            case MessageType::CHANGE_SONG:
                INFO("Listener recieved CHANGE_SONG");
                break;
            case MessageType::DISCONNECT:
                INFO("Listener recieved DISCONNECT");
                break;
            case MessageType::CONNECT:
                INFO("Listener recieved CONNECT");
                break;
            case MessageType::QUERY_STATUS:
                INFO("Listener recieved QUERY_STATUS");
                break;
            case MessageType::SEND_STATUS:
                INFO("Listener recieved SEND_STATUS");
                break;
            case MessageType::BULLY:
                INFO("Listenere recieved BULLY");
                break;
            case MessageType::CONNECTOR_LIST:
                INFO("Listener recieved CONNECTOR_LIST");
                break;
            case MessageType::CONNECTOR_LIST_UPDATE:
                INFO("Listener recieved CONNECTOR_LIST_UPDATE");
                break;
        }
        return false;
    }

    static void start_listener_server(network& net, std::span<char> rb, std::span<char> wb)
    {
        std::function server = [](std::span<char> read_buf, std::span<char> write_buf, const std::string& ip, std::uint16_t port) -> bool
        {
            Message msg = read_message_from_buffer(read_buf);
            return message_handler(msg);
        };
        net.async_create_server(LISTENER_SERVER_PORT, rb, wb, server);
    }

    void start_listener(const std::string_view connector_ip, const uint16_t connector_port)
    {
        INFO("Starting listener");

        player::MusicPlayer player(0);

        std::array<char, 1024> client_rb;
        std::array<char, 1024> client_wb;
        std::array<char, 1024> server_rb;
        std::array<char, 1024> server_wb;

        network net;

        const std::string leader_ip = "";
        const uint16_t leader_port = 5555;

        Message connect_msg = { MessageType::CONNECT };
        start_listener_server(net, server_rb, server_wb);
        send_message_to_leader(connect_msg, net, connector_ip, connector_port, client_rb, client_wb);

        net.run();

    }

}
